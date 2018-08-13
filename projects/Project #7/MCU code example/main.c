#include "stm32f10x_conf.h"
#include <stdio.h>

#define TOGGLE_LED()		GPIO_Write(GPIOC, GPIO_ReadOutputData(GPIOC) ^ GPIO_Pin_13)

#define DS1307_ADDRESS		0xD0

#define BCD2DEC(x)			((x >> 4) * 10 + (x & 0x0F))
#define DEC2BCD(x)			(((x / 10) << 4) + (x % 10))

#define DS_WIRES_COUNT		6
#define PERIOD_BYTE			0x08
#define RESOLUTION_BYTE		0x09
#define WSTATUS_BYTE		0x0A

typedef enum {
	M_NORMAL_SCAN	= 0x00,
	M_FAST_SCAN
} MCU_Mode;

typedef enum {
	S_MCU_DOWNTIME	= 0x00,
	S_MCU_START_CONV,
	S_MCU_GRAB_DATA,
	S_MCU_WRITE_DATA,
	S_MCU_ANSWER_PC,
	S_MCU_SCAN_ROMS,
	#ifdef USE_BMx_SENSOR
	S_MCU_BMx_COMMUNICATION
	#endif
} MCU_Status;

typedef enum {
	T_NO_TASK		= 0x00,
	T_GRAB_DATA		= 0x01,
	T_START_CONV	= 0x02,
	T_WRITE_DATA	= 0x04,
	T_ANSWER_PC		= 0x08,
	T_SCAN_ROMS		= 0x10,
	#ifdef USE_BMx_SENSOR
	T_GET_BMx_DATA	= 0x20
	#endif
} MCU_Task;

#define IS_MCU_MODE(x)		(x == M_NORMAL_SCAN || x == M_FAST_SCAN)
#define IS_CYCLE_PERIOD(x)	(x > 0)
#define IS_WIRE_STATUS(x)	(x == 0 || x == 1)

FATFS			fs;
FRESULT			LastFSState		= FR_OK;

MCU_Status		DeviceStatus	= S_MCU_DOWNTIME;
MCU_Task 		DeviceTask		= T_NO_TASK;
MCU_Mode		DeviceMode		= M_NORMAL_SCAN;

DS18B20_Wire	ds_wires[DS_WIRES_COUNT];
WRESULT			LastDSState		= W_OK;
uint8_t			SecCounter		= 0;
uint8_t			MinCounter		= 0;
uint8_t			CyclePeriod		= 1;
uint8_t			DSResolution	= DS18B20_Resolution_12_bit;
uint8_t			WireToScanROMs	= 0;

#ifdef USE_BMx_SENSOR
BMx280_Data		bmx_data;
#endif

/* ************************************* */

char sd_buffer[50] = {'\0'}, sd_file[20] = {'\0'};
uint8_t sd_buf_size = 0;
uint8_t ds_first_record = 1;
#ifdef USE_BMx_SENSOR
uint8_t bmx_first_record = 1;
#endif

uint8_t sec, min, hour, wday, mday, mon, year;

uint32_t systick_us = 0;

uint8_t hid_tx_buffer[ENDP1_IN_SIZE+1] = {'\0'};
uint8_t hid_rx_buffer[ENDP1_OUT_SIZE+1] = {'\0'};
uint8_t PrevXferComplete = 1;

/* ************************************* */

void HID_SendBuffer(void);

void delay_us(uint32_t us) {
	uint32_t init_us = systick_us;
	while(systick_us - init_us < us);
}

void fill_hid_with_zeros() {
	uint8_t i;
	for (i = 0; i <= ENDP1_IN_SIZE; i++) hid_tx_buffer[i] = 0;
}

void init_LED(void) {
	GPIO_InitTypeDef port;
	
	GPIO_StructInit(&port);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	port.GPIO_Pin	= GPIO_Pin_13;
	port.GPIO_Mode	= GPIO_Mode_Out_PP;
	port.GPIO_Speed	= GPIO_Speed_2MHz;
	
	GPIO_Init(GPIOC, &port);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void init_timer3(void) {
	TIM_TimeBaseInitTypeDef timer3;
	
	TIM_TimeBaseStructInit(&timer3);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	timer3.TIM_Prescaler = 36000-1;
	timer3.TIM_Period = 2000 - 1;
	timer3.TIM_ClockDivision = TIM_CKD_DIV1;
	timer3.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM3, &timer3);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	
	NVIC_EnableIRQ(TIM3_IRQn);
}

void init_i2c1(void) {
	GPIO_InitTypeDef gpio;
	I2C_InitTypeDef i2c;
	
	GPIO_StructInit(&gpio);
	I2C_StructInit(&i2c);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
	
	gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &gpio);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	
	i2c.I2C_ClockSpeed = 100000;
	i2c.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_OwnAddress1 = 1;
	i2c.I2C_Ack = I2C_Ack_Enable;
	i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(I2C1, &i2c);
	
	I2C_Cmd(I2C1, ENABLE);
}

void init_ds18b20(void) {
	GPIO_InitTypeDef gpio;
	uint8_t i, tries, t;
	
	GPIO_StructInit(&gpio);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
	
	// wire 0 and 1
	gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_15;
	gpio.GPIO_Mode = GPIO_Mode_Out_OD;
	gpio.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &gpio);
	GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_15);
	
	// wires 2 - 5
	gpio.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_Out_OD;
	gpio.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &gpio);
	GPIO_SetBits(GPIOB, GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_7);
	
	i = 0;
	ds18b20_wire_init(&ds_wires[i++], GPIOA, GPIO_Pin_8);
	ds18b20_wire_init(&ds_wires[i++], GPIOA, GPIO_Pin_15);
	ds18b20_wire_init(&ds_wires[i++], GPIOB, GPIO_Pin_3);
	ds18b20_wire_init(&ds_wires[i++], GPIOB, GPIO_Pin_4);
	ds18b20_wire_init(&ds_wires[i++], GPIOB, GPIO_Pin_6);
	ds18b20_wire_init(&ds_wires[i++], GPIOB, GPIO_Pin_7);
	
	t = I2C_single_read(I2C1, DS1307_ADDRESS, RESOLUTION_BYTE);
	if (IS_DS_RESOLUTION(t)) DSResolution = t;
	
	LastDSState = W_OK;
	
	for (i = 0; i < DS_WIRES_COUNT; i++) {
		t = I2C_single_read(I2C1, DS1307_ADDRESS, WSTATUS_BYTE+i);
		if (IS_WIRE_STATUS(t)) ds_wires[i].activated = t;
		
		if ( !ds_wires[i].activated ) continue;
		
		for (tries = 5; tries > 0; tries--) {
			ds_wires[i].state = ds18b20_search_rom(&ds_wires[i]);
			
			if (ds_wires[i].state == W_OK) break;
			delay_us(50);
		}
		
		ds_wires[i].state |= ds18b20_set_resolution(&ds_wires[i], DSResolution);
		LastDSState |= ds_wires[i].state;
	}
	DeviceTask |= T_START_CONV;
}

void init_USB_HID(void) {
	Set_System();
    USB_Interrupts_Config();
    Set_USBClock();
    USB_Init();
}


void init_all(void) {
	uint8_t t;
	
	init_LED();
	init_timer3();
	
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	SysTick_Config(72);
	
	init_i2c1();
	
	// setting up a period of cycle operation
	t = I2C_single_read(I2C1, DS1307_ADDRESS, PERIOD_BYTE);
	if (IS_CYCLE_PERIOD(t)) CyclePeriod = t;
	
	// check if the rcc chip initialized and toggle led if not
	t = I2C_single_read(I2C1, DS1307_ADDRESS, 0x00);
	if (t & 0x80) TOGGLE_LED();
	
	#ifdef USE_BMx_SENSOR
	BMx280_InitStruct(&bmx_data);
	BMx280_Init();
	#endif
	
	init_ds18b20();
	init_USB_HID();
}

__inline void update_time(void) {
	sec		= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x00));		// 0-59
	min		= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x01));		// 0-59
	hour	= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x02));		// 0-24
	
	wday	= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x03));		// 1-7
	mday	= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x04));		// 1-31
	mon		= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x05));		// 1-12
	year	= BCD2DEC(I2C_single_read(I2C1, DS1307_ADDRESS, 0x06));		// 0-99 (+ 2000)
}

FRESULT ds_make_record_in_file(const char * filename, DS18B20_Wire * wire) {
	FIL file;
	FRESULT result;
	UINT len;
	uint8_t i, j;
	float temp;
	
	if (wire->devices <= 0) return FR_OK;
	
	result = f_mount(&fs, "0", 1);
	if (result != FR_OK) return result;
	
	result = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
	if (result != FR_OK) return result;
	
	result = f_lseek(&file, file.fsize);
	if (result != FR_OK) return result;
	
	if (ds_first_record || file.fsize == 0) {
		ds_first_record = 0;
		sd_buf_size = sprintf(sd_buffer, "%02d/%02d/20%02d", mday, mon, year);
		
		result = f_write(&file, sd_buffer, sd_buf_size, &len);
		if (result != FR_OK) return result;
		
		for (i = 0; i < wire->devices; i++) {
			result = f_write(&file, ";R", 2, &len);
			if (result != FR_OK) return result;
			
			for (j = 0; j < 7; j++) {
				sd_buf_size = sprintf(sd_buffer, "%02X", wire->dev[i].ROM[j]);
		
				result = f_write(&file, sd_buffer, sd_buf_size, &len);
				if (result != FR_OK) return result;
			}
		}
		
		result = f_write(&file, "\r\n", 2, &len);
		if (result != FR_OK) return result;
	}
	
	sd_buf_size = sprintf(sd_buffer, "%02d:%02d:%02d", hour, min, sec);
	
	result = f_write(&file, sd_buffer, sd_buf_size, &len);
	if (result != FR_OK) return result;
		
	for (i = 0; i < wire->devices; i++) {
		temp = (float)( ((wire->dev[i].SCRATCHPAD[1] << 8) | (wire->dev[i].SCRATCHPAD[0])) / 16.0 );
		sd_buf_size = sprintf(sd_buffer, ";%+.2f", temp);
	
		result = f_write(&file, sd_buffer, sd_buf_size, &len);
		if (result != FR_OK) return result;
	}
	
	result = f_write(&file, "\r\n", 2, &len);
	if (result != FR_OK) return result;
	
	result = f_close(&file);
	if (result != FR_OK) return result;
	
	result = f_mount(NULL, "0", 1);
	if (result != FR_OK) return result;
	
	return FR_OK;
}

#if defined(USE_BMx_SENSOR) && defined(SAVE_BMx_TO_SD)
FRESULT bmx_make_record_in_file(const char * filename, BMx280_Data * data) {
	FIL file;
	FRESULT result;
	UINT len;
	
	result = f_mount(&fs, "0", 1);
	if (result != FR_OK) return result;
	
	result = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
	if (result != FR_OK) return result;
	
	result = f_lseek(&file, file.fsize);
	if (result != FR_OK) return result;
	
	if (bmx_first_record || file.fsize == 0) {
		bmx_first_record = 0;
		
		sd_buf_size = sprintf(sd_buffer, "%02d/%02d/20%02d", mday, mon, year);
		result = f_write(&file, sd_buffer, sd_buf_size, &len);
		if (result != FR_OK) return result;
		
		result = f_write(&file, ";T;P", 4, &len);
		if (result != FR_OK) return result;
		
		#ifdef BME_DEVICE
		result = f_write(&file, ";RH", 3, &len);
		if (result != FR_OK) return result;
		#endif
		
		result = f_write(&file, "\r\n", 2, &len);
		if (result != FR_OK) return result;
	}
	
	sd_buf_size = sprintf(sd_buffer, "%02d:%02d:%02d", hour, min, sec);
	
	result = f_write(&file, sd_buffer, sd_buf_size, &len);
	if (result != FR_OK) return result;
	
	sd_buf_size = sprintf(sd_buffer, ";%+.2f;%.2f", data->temperature, data->pressure);
	result = f_write(&file, sd_buffer, sd_buf_size, &len);
	if (result != FR_OK) return result;
	
	#ifdef BME_DEVICE
	sd_buf_size = sprintf(sd_buffer, ";%.2f", data->humidity);
	result = f_write(&file, sd_buffer, sd_buf_size, &len);
	if (result != FR_OK) return result;
	#endif
	
	result = f_write(&file, "\r\n", 2, &len);
	if (result != FR_OK) return result;
	
	result = f_close(&file);
	if (result != FR_OK) return result;
	
	result = f_mount(NULL, "0", 1);
	if (result != FR_OK) return result;
	
	return FR_OK;
}
#endif

int main() {
	uint8_t i, j, arg, dev;
	#ifdef USE_BMx_SENSOR
	uint8_t * p_byte = 0;
	#endif
	
	uint32_t DS_start_conv_time = 0;
	uint8_t  DS_cycle_started	= 0;
	
	init_all();
	
	DeviceStatus = S_MCU_DOWNTIME;
	
    while(1) {
		// count minutes and run cycle after 'minutes' reached cycle period
		if ( MinCounter >= CyclePeriod ) {
			MinCounter = 0;
			if (DeviceMode != M_FAST_SCAN) DeviceTask |= T_START_CONV;
		}
		// add task to read data if mcu waited enough for conversion time
		if ( DS_cycle_started & (systick_us - DS_start_conv_time >= DELAY_T_CONVERT) ) {
			DeviceTask |= T_GRAB_DATA;
		}
		
		// grab data from all DS wires
		if ( DeviceTask & T_GRAB_DATA ) {
			DeviceStatus = S_MCU_GRAB_DATA;
			LastDSState = W_OK;
			
			for (i = 0; i < DS_WIRES_COUNT; i++) {
				if ( !ds_wires[i].activated ) continue;
				ds_wires[i].state = ds18b20_read_all_scratchpad(&ds_wires[i]);
				LastDSState |= ds_wires[i].state;
			}
			
			// add task to write data
			if (DeviceMode != M_FAST_SCAN) DeviceTask |= T_WRITE_DATA;
			// and reset cycle
			DS_cycle_started = 0;
			
			// perform receiving data from BMx280 with the same frequency as for DS
			#ifdef USE_BMx_SENSOR
			DeviceTask |= T_GET_BMx_DATA;
			#endif
			
			DeviceTask &= ~T_GRAB_DATA;
			DeviceStatus = S_MCU_DOWNTIME;
		}
		
		// send command 'start conversion' to all DS wires
		if ( DeviceTask & T_START_CONV ) {
			DeviceStatus = S_MCU_START_CONV;
			LastDSState = W_OK;
			
			for (i = 0; i < DS_WIRES_COUNT; i++) {
				if ( !ds_wires[i].activated ) continue;
				ds_wires[i].state = ds18b20_start_conversion(&ds_wires[i]);
				LastDSState |= ds_wires[i].state;
			}
			// remember the time when conversion started
			DS_start_conv_time = systick_us;
			// and set flag that conversion has been started
			DS_cycle_started = 1;
			
			DeviceTask &= ~T_START_CONV;
			DeviceStatus = S_MCU_DOWNTIME;
		}
		
		// get data from bmx sensor
		#ifdef USE_BMx_SENSOR
		if ( DeviceTask & T_GET_BMx_DATA ) {
			DeviceStatus = S_MCU_BMx_COMMUNICATION;
			
			BMx280Convert(&bmx_data);
			
			DeviceTask &= ~T_GET_BMx_DATA;
			DeviceStatus = S_MCU_DOWNTIME;
		}
		#endif
		
		// perform saving data onto SD card
		if ( DeviceTask & T_WRITE_DATA ) {
			TOGGLE_LED();
			DeviceStatus = S_MCU_WRITE_DATA;
			LastFSState = FR_OK;
			
			update_time();
			
			for (i = 0; i < DS_WIRES_COUNT; i++) {
				if ( !ds_wires[i].activated ) continue;
				sprintf(sd_file, "W%d_%02d%02d.csv", i+1, mon, mday);
				LastFSState |= ds_make_record_in_file(sd_file, &ds_wires[i]);
			}
			
			#if defined(USE_BMx_SENSOR) && defined(SAVE_BMx_TO_SD)
			sprintf(sd_file, "Bx_%02d%02d.csv", mon, mday);
			LastFSState |= bmx_make_record_in_file(sd_file, &bmx_data);
			#endif
			
			DeviceTask &= ~T_WRITE_DATA;
			DeviceStatus = S_MCU_DOWNTIME;
			TOGGLE_LED();
		}
		
		// answer for PC USB request
		if ( DeviceTask & T_ANSWER_PC ) {
			DeviceStatus = S_MCU_ANSWER_PC;
			
			arg = 0;
			
			switch (hid_rx_buffer[0]) {
				// return time to PC in format [SEND_TIME, sec, min, hour, wday, mday, mon, year]
				case GET_TIME:
					fill_hid_with_zeros();
					update_time();
					
					i = 0;
					hid_tx_buffer[i++] = SEND_TIME;
					hid_tx_buffer[i++] = sec;
					hid_tx_buffer[i++] = min;
					hid_tx_buffer[i++] = hour;
					hid_tx_buffer[i++] = wday;
					hid_tx_buffer[i++] = mday;
					hid_tx_buffer[i++] = mon;
					hid_tx_buffer[i++] = year;
					HID_SendBuffer();
					break;
				
				// setup time from PC according to format: [SET_TIME, sec, min, hour, wday, mday, mon, year]
				case SET_TIME:
					for (i = 0; i < 7; i++) {
						I2C_single_write(I2C1, DS1307_ADDRESS, i, DEC2BCD(hid_rx_buffer[i + 1]) );
					}
					GPIO_SetBits(GPIOC, GPIO_Pin_13);
					break;
				
				// setup scanning mode: NORMAL or FAST
				case SET_MODE:
					arg = hid_rx_buffer[1];
					if (IS_MCU_MODE(arg)) {
						DeviceMode = arg;
					}
					break;
				
				// setup new CyclePeriod in eeprom and in workspace
				case SET_PERD:
					arg = hid_rx_buffer[1];
					if (IS_CYCLE_PERIOD(arg)) {
						I2C_single_write(I2C1, DS1307_ADDRESS, PERIOD_BYTE, arg);
						CyclePeriod = arg;
					}
					break;
				
				// send last info in format [SEND_LINF, DeviceMode, LastFSState, LastDSState, CyclePeriod, SecCounter, MinCounter]
				case GET_LINF:
					arg = CyclePeriod;
					fill_hid_with_zeros();
				
					i = 0;
					hid_tx_buffer[i++] = SEND_LINF;
					hid_tx_buffer[i++] = DeviceMode;
					hid_tx_buffer[i++] = LastFSState;
					hid_tx_buffer[i++] = LastDSState;
					hid_tx_buffer[i++] = CyclePeriod;
					hid_tx_buffer[i++] = SecCounter;
					hid_tx_buffer[i++] = MinCounter;
					hid_tx_buffer[i++] = DSResolution;
					hid_tx_buffer[i++] = DS_WIRES_COUNT;
					#if defined(USE_BMx_SENSOR) && defined(SAVE_BMx_TO_SD) && defined(BME_DEVICE)
					hid_tx_buffer[i++] = 1 + 2 + 4;
					#elif defined(USE_BMx_SENSOR) && defined(SAVE_BMx_TO_SD)
					hid_tx_buffer[i++] = 1 + 2;
					#elif defined(USE_BMx_SENSOR)
					hid_tx_buffer[i++] = 1;
					#else
					hid_tx_buffer[i++] = 0;
					#endif
				
					HID_SendBuffer();
					break;
				
				// set wire status
				case SET_WSTS:
					arg = hid_rx_buffer[1];
					if (arg < DS_WIRES_COUNT) {
						if (IS_WIRE_STATUS(hid_rx_buffer[2])) {
							if (!ds_wires[arg].activated) {
								WireToScanROMs = arg;
								DeviceTask |= T_SCAN_ROMS;
							}
							I2C_single_write(I2C1, DS1307_ADDRESS, WSTATUS_BYTE + arg, hid_rx_buffer[2]);
							ds_wires[arg].activated = hid_rx_buffer[2];
							ds18b20_set_resolution(&ds_wires[arg], DSResolution);
						}
					}
					break;
				
				// send ROM codes from specified wire
				case GET_ROMS:
					arg = hid_rx_buffer[1];
					fill_hid_with_zeros();
				
					i = 0;
					hid_tx_buffer[i++] = SEND_ROMS;
					if (arg < DS_WIRES_COUNT) {
						hid_tx_buffer[i++] = ds_wires[arg].devices;
						hid_tx_buffer[i++] = ds_wires[arg].activated;
						hid_tx_buffer[i++] = ds_wires[arg].state;
						
						for (dev = 0; dev < ds_wires[arg].devices; dev++) {
							for (j = 0; j < 7; j++) {
								hid_tx_buffer[i++] = ds_wires[arg].dev[dev].ROM[j];
							}
						}
					}
					HID_SendBuffer();
					break;
				
				// read all devices memory from specialized wire
				case GET_DMEM:
					arg = hid_rx_buffer[1];
					fill_hid_with_zeros();
				
					i = 0;
					hid_tx_buffer[i++] = SEND_DMEM;
					if (arg < DS_WIRES_COUNT) {
						hid_tx_buffer[i++] = ds_wires[arg].devices;
						hid_tx_buffer[i++] = ds_wires[arg].activated;
						hid_tx_buffer[i++] = ds_wires[arg].state;
						
						for (dev = 0; dev < ds_wires[arg].devices; dev++) {
							for (j = 0; j < 5; j++) {
								hid_tx_buffer[i++] = ds_wires[arg].dev[dev].SCRATCHPAD[j];
							}
						}
					}
					HID_SendBuffer();
					break;
				
				// send temp to PC, 2 byte per one sensor
				case GET_TEMP:
					arg = hid_rx_buffer[1];
					fill_hid_with_zeros();
				
					i = 0;
					hid_tx_buffer[i++] = SEND_TEMP;
					if (arg < DS_WIRES_COUNT) {
						hid_tx_buffer[i++] = ds_wires[arg].devices;
						hid_tx_buffer[i++] = ds_wires[arg].activated;
						hid_tx_buffer[i++] = ds_wires[arg].state;
						
						for (dev = 0; dev < ds_wires[arg].devices; dev++) {
							for (j = 0; j < 2; j++) {
								hid_tx_buffer[i++] = ds_wires[arg].dev[dev].SCRATCHPAD[j];
							}
						}
					}
					HID_SendBuffer();
					break;
					
				// add task to scan wire roms
				case SCAN_WIRE:
					arg = hid_rx_buffer[1];
					if (arg < DS_WIRES_COUNT) {
						WireToScanROMs = arg;
						DeviceTask |= T_SCAN_ROMS;
					}
					break;
					
					
				// set new resolution of sensor
				case SET_RESL:
					arg = hid_rx_buffer[1];
					if (IS_DS_RESOLUTION(arg)) {
						I2C_single_write(I2C1, DS1307_ADDRESS, RESOLUTION_BYTE, arg);
						DSResolution = arg;
						
						for (i = 0; i < DS_WIRES_COUNT; i++) {
							if (ds_wires[i].activated) ds18b20_set_resolution(&ds_wires[i], DSResolution);
						}
					}
					break;
					
				// send BMx280 data
				#ifdef USE_BMx_SENSOR
				case GET_BMx:
					fill_hid_with_zeros();

					i = 0;
					hid_tx_buffer[i++] = SEND_BMx;
				
					p_byte = (uint8_t *)&bmx_data.temperature;
					for (j = 0; j < 4; j++) hid_tx_buffer[i++] = p_byte[j];
					p_byte = (uint8_t *)&bmx_data.pressure;
					for (j = 0; j < 4; j++) hid_tx_buffer[i++] = p_byte[j];
					#ifdef BME_DEVICE
					p_byte = (uint8_t *)&bmx_data.humidity;
					for (j = 0; j < 4; j++) hid_tx_buffer[i++] = p_byte[j];
					#endif
				
					HID_SendBuffer();
					break;
				#endif
					
				// unknown or wrong command
				default:
					fill_hid_with_zeros();
					hid_tx_buffer[0] = WRONG_CMD;
					HID_SendBuffer();
					break;
			}
			
			DeviceTask &= ~T_ANSWER_PC;
			DeviceStatus = S_MCU_DOWNTIME;
		}
		
		// start scanning ROM codes on specified wire
		if ( DeviceTask & T_SCAN_ROMS ) {
			DeviceStatus = S_MCU_SCAN_ROMS;
			
			ds18b20_search_rom(&ds_wires[WireToScanROMs]);
			
			DeviceTask &= ~T_SCAN_ROMS;
			DeviceStatus = S_MCU_DOWNTIME;
		}
	}
}

void HID_SendBuffer(void) {
	if (PrevXferComplete && (bDeviceState == CONFIGURED)) {
		USB_SIL_Write(EP1_IN, hid_tx_buffer, ENDP1_IN_SIZE);
		PrevXferComplete = 0;
		SetEPTxValid(ENDP1);
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void) {
	USB_Istr();
}

void USBWakeUp_IRQHandler(void) {
	EXTI_ClearITPendingBit(EXTI_Line18);
}

void EP1_OUT_Callback(void) {
	uint8_t i;
	
	USB_SIL_Read(EP1_OUT, hid_rx_buffer);
	
	if (hid_rx_buffer[0] == GET_MSTS) {
		i = 0;
		hid_tx_buffer[i++] = SEND_MSTS;
		hid_tx_buffer[i++] = DeviceStatus;
		hid_tx_buffer[i++] = DeviceMode;
		hid_tx_buffer[i++] = DeviceTask;
		hid_tx_buffer[i++] = CyclePeriod;
		hid_tx_buffer[i++] = DSResolution;
		hid_tx_buffer[i++] = DS_WIRES_COUNT;
		#if defined(USE_BMx_SENSOR) && defined(SAVE_BMx_TO_SD) && defined(BME_DEVICE)
		hid_tx_buffer[i++] = 1 + 2 + 4;
		#elif defined(USE_BMx_SENSOR) && defined(SAVE_BMx_TO_SD)
		hid_tx_buffer[i++] = 1 + 2;
		#elif defined(USE_BMx_SENSOR)
		hid_tx_buffer[i++] = 1;
		#else
		hid_tx_buffer[i++] = 0;
		#endif
		HID_SendBuffer();
	} else {
		DeviceTask |= T_ANSWER_PC;
	}

	SetEPRxStatus(ENDP1, EP_RX_VALID);
}

void EP1_IN_Callback(void) {
	PrevXferComplete = 1;
}

void SysTick_Handler(void) {
	systick_us++;
}

void TIM3_IRQHandler(void) {
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	
	if (DeviceMode == M_FAST_SCAN) {
		DeviceTask |= T_START_CONV;
	}
	
	if (++SecCounter == 60) {
		SecCounter = 0;
		MinCounter++;
	}
}
