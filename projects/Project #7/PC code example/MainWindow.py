# -*- coding: UTF-8 -*-

from PyQt5 import QtCore, QtWidgets, QtGui
from enum import Enum, unique
import usb.core
from usb.backend import libusb1
import usb.util
import queue
import sys
import time

import struct

#import matplotlib
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib import dates
from matplotlib.ticker import MaxNLocator, FormatStrFormatter

from functools import partial
from datetime import datetime

import platform
import random


# interface thread with its logic
class MCUInterface(QtCore.QThread):
    VID = # ---- CUT! ----
    PID = # ---- CUT! ----

    device = None
    ep_in = None
    ep_out = None

    # high level abstraction variables
    device_wires = None
    device_mode = None
    device_period = None
    device_resolution = None

    bmx_state = None

    # string signals
    send_status = QtCore.pyqtSignal(str, int)
    
    # signals for initializing work with device
    device_connected = QtCore.pyqtSignal()
    info_received = QtCore.pyqtSignal(list, int, int, int, int)  # wires / mode / period / resolution / bmx_state
    device_disconnected = QtCore.pyqtSignal()

    # interface signals
    mcu_status = QtCore.pyqtSignal(list)
    mcu_time = QtCore.pyqtSignal(list)
    mcu_last_info = QtCore.pyqtSignal(list)
    mcu_roms = QtCore.pyqtSignal(int, list, list)
    mcu_temp = QtCore.pyqtSignal(int, list, list)
    mcu_device_memory = QtCore.pyqtSignal(int, list, list)
    mcu_bmx_data = QtCore.pyqtSignal(list)
    
    # special notification signals
    wire_roms_changed = QtCore.pyqtSignal(int, list, list)  # wire / old roms / new roms
    wire_status_changed = QtCore.pyqtSignal(int, int, int)  # wire / old status / new status
    wire_devices_changed = QtCore.pyqtSignal(int, int, int)  # wire / old devices / new devices
    wire_active_changed = QtCore.pyqtSignal(int, int, int)  # wire / old activity / new activity
    wires_resolution_changed = QtCore.pyqtSignal(int, int, int)  # wire / old resolution / new resolution

    # queue
    queue = queue.Queue()
    thread_active = True
    __device_connected = False

    read_packet_size = # ---- CUT! ----

    # mcu timings
    class MCUTimings(Enum):
        Read =  # ---- CUT! ----
        Write = # ---- CUT! ----
        Sleep = # ---- CUT! ----

    # mcu commands enum
    @unique
    class MCUCommands(Enum):
        GET_TIME = # ---- CUT! ----
        SET_TIME = # ---- CUT! ----
        SET_MODE = # ---- CUT! ----
        SET_PERD = # ---- CUT! ----
        GET_MSTS = # ---- CUT! ----
        GET_LINF = # ---- CUT! ----
        SET_WSTS = # ---- CUT! ----
        GET_ROMS = # ---- CUT! ----
        GET_DMEM = # ---- CUT! ----
        GET_TEMP = # ---- CUT! ----
        SCAN_WIRE = # ---- CUT! ----
        SET_RESL = # ---- CUT! ----
        GET_BMx = # ---- CUT! ----

    # mcu answer codes
    @unique
    class MCUAnswers(Enum):
        WRONG_CMD = # ---- CUT! ----
        SEND_TIME = # ---- CUT! ----
        SET_NTIME = # ---- CUT! ----
        SET_NMODE = # ---- CUT! ----
        SET_NPERD = # ---- CUT! ----
        SEND_MSTS = # ---- CUT! ----
        SEND_LINF = # ---- CUT! ----
        SET_NWSTS = # ---- CUT! ----
        SEND_ROMS = # ---- CUT! ----
        SEND_DMEM = # ---- CUT! ----
        SEND_TEMP = # ---- CUT! ----
        SET_NRESL = # ---- CUT! ----
        SEND_BMx =  # ---- CUT! ----

    # mcu status codes
    @unique
    class MCUStatus(Enum):
        DOWNTIME = # ---- CUT! ----
        START_CONV = # ---- CUT! ----
        GRAB_DATA = # ---- CUT! ----
        WRITE_DATA = # ---- CUT! ----
        ANSWER_PC = # ---- CUT! ----
        SCAN_ROMS = # ---- CUT! ----
        S_MCU_BMx_COMMUNICATION = # ---- CUT! ----

    # mcu modes
    @unique
    class MCUModes(Enum):
        NORMAL_SCAN = # ---- CUT! ----
        FAST_SCAN = # ---- CUT! ----

    # mcu tasks
    @unique
    class MCUTasks(Enum):
        NO_TASK = # ---- CUT! ----
        GRAB_DATA = # ---- CUT! ----
        START_CONV = # ---- CUT! ----
        WRITE_DATA = # ---- CUT! ----
        ANSWER_PC = # ---- CUT! ----
        SCAN_ROMS = # ---- CUT! ----
        T_GET_BMx_DATA = # ---- CUT! ----

    @unique
    class BMx280States(Enum):
        USE_BMx_SENSOR = # ---- CUT! ----
        SAVE_BMx_TO_SD = # ---- CUT! ----
        BME_DEVICE = # ---- CUT! ----

    # ds resolutions
    @unique
    class MCUResolutions(Enum):
        R9_BIT  = 0x1F
        R10_BIT = 0x3F
        R11_BIT = 0x5F
        R12_BIT = 0x7F

    # hardcoded states of DS operations
    DSStates = {
        0x00:   'Ок',
        0x01:   'Подтягивающий резистор отсутствует',
        0x02:   'Нет устройств',
        0x04:   'Максимальное количество устройств на шине',
        0x08:   'Ошибка при проверке ROM CRC',
        0x10:   'Ошибка при проверке SCR CRC',
        0x20:   'Ошибка при поиске устройтв "11"',
        0x40:   'Неверное устройство'
    }

    # hardcoded states of FS operations
    FSStates = {
        0:  'Ок',
        1:  'Ошибка диска',
        2:  'Внутренняя ошибка',
        3:  'Диск не готов к работе',
        4:  'Несуществующий файл',
        5:  'Несуществующий путь',
        6:  'Неверное имя',
        7:  'Доступ запрещен',
        8:  'Объект существует',
        9:  'Неверный объект',
        10: 'Диск защищен от записи',
        11: 'Неверное устройство',
        12: 'Не разрешено',
        13: 'Нет файловой системы',
        14: 'mkfs прервана',
        15: 'Истекло время ожидания',
        16: 'Устройство заблокировано',
        17: 'Не достсточно места',
        18: 'Много открытых файлов',
        19: 'Неверный параметр'
    }

    # functions
    def __init__(self, parent=None):
        QtCore.QThread.__init__(self, parent)

        self.backend = libusb1.get_backend()

        self.device_connected.connect(self.device_info_receive)

    # initialize a connection with device
    def mcu_connect(self):
        print('mcu_connect')
        try:
            # searching for device
            device = usb.core.find(idVendor=self.VID, idProduct=self.PID, backend=self.backend)
            if device is None:
                raise ValueError('MCU device is NoneType')

            # device found. if having deal with linux we need to detach its driver
            if 'linux' in platform.system().casefold():
                if device.is_kernel_driver_active(0):
                    device.detach_kernel_driver(0)

            # then configure the connected device
            device.set_configuration()
            cfg = device.get_active_configuration()

            # and find two endpoints (IN and OUT) for following opeartions
            ep_out = usb.util.find_descriptor(
                cfg[(0, 0)],
                custom_match=lambda e:
                        usb.util.endpoint_direction(e.bEndpointAddress) ==
                        usb.util.ENDPOINT_OUT)

            if ep_out is None:
                raise ValueError('OUT endpoint is not found!')

            ep_in = usb.util.find_descriptor(
                cfg[(0, 0)],
                custom_match=lambda e:
                        usb.util.endpoint_direction(e.bEndpointAddress) ==
                        usb.util.ENDPOINT_IN)

            if ep_in is None:
                raise ValueError('IN endpoint is not found')

            self.device = device
            self.ep_in = ep_in
            self.ep_out = ep_out
            self.__device_connected = True
            return True

        except ValueError as e:
            return False

    # serves for sending data to device
    def mcu_write(self, command, data, wire=None):
        if wire is not None:
            data.insert(0, wire)

        data.insert(0, command)
        self.ep_out.write(bytes(data), self.MCUTimings.Write.value)

    # serves for receiving data from device
    def mcu_read(self):
        res = self.ep_in.read(self.read_packet_size, self.MCUTimings.Read.value)
        return res

    # upper level function serves for requering data from device with its verification
    def mcu_write_read(self, command, data, wire=None):
        self.mcu_write(command, data, wire)
        self.msleep(self.MCUTimings.Sleep.value)
        res = self.mcu_read()

        # different checks if the res correct or not
        if res[0] != command:
            raise ValueError('Неожиданный ответ устройства на команду %s: %d!' % (self.MCUCommands(command).name, res[0]))

        return res

    # mcu interface commands
    def get_mcu_status(self):
        r = list(self.mcu_write_read(self.MCUCommands.GET_MSTS.value, [])[1:8])
        self.mcu_status.emit(r)
        return r

    def get_time(self):
        r = list(self.mcu_write_read(self.MCUCommands.GET_TIME.value, [])[1:8])
        self.mcu_time.emit(r)
        return r

    def set_time(self):
        cur_time = time.localtime()
        data = [cur_time.tm_sec,
                cur_time.tm_min,
                cur_time.tm_hour,
                cur_time.tm_wday + 1,
                cur_time.tm_mday,
                cur_time.tm_mon,
                cur_time.tm_year - 2000]
        self.mcu_write(self.MCUCommands.SET_TIME.value, data)

    def set_mode(self, mode):
        self.mcu_write(self.MCUCommands.SET_MODE.value, [mode.value])

    def set_period(self, period):
        self.mcu_write(self.MCUCommands.SET_PERD.value, [period])

    def get_last_info(self):
        r = list(self.mcu_write_read(self.MCUCommands.GET_LINF.value, [])[1:9])
        self.mcu_last_info.emit(r)
        return r

    def update_wire_info(self, wire, info_list):
        # if wire is already in use
        if self.device_wires is not None and wire < len(self.device_wires):
            # number of devices changed
            if info_list[0] != self.device_wires[wire]['devices']:
                old_value = self.device_wires[wire]['devices']
                self.device_wires[wire]['devices'] = info_list[0]
                self.wire_devices_changed.emit(wire, old_value, info_list[0])
                
            # wire activity changed
            if info_list[1] != self.device_wires[wire]['active']:
                old_value = self.device_wires[wire]['active']
                self.device_wires[wire]['active'] = info_list[1]
                self.wire_active_changed.emit(wire, old_value, info_list[1])
                
            # wire status changed
            if info_list[2] != self.device_wires[wire]['status']:
                old_value = self.device_wires[wire]['status']
                self.device_wires[wire]['status'] = info_list[2]
                self.wire_status_changed.emit(wire, old_value, info_list[2])
    
    # returns rom codes from specified wire if no errors met
    def get_roms(self, wire):
        r = list(self.mcu_write_read(self.MCUCommands.GET_ROMS.value, [], wire=wire))
        roms = []
        for i in range(r[1]):
            s = ''
            for j in range(7):
                s += '%02X' % r[4+i*7+j]
            roms.append(s)
            
        # update an information about wires inside the interface class object
        self.update_wire_info(wire, r[1:4])
        if self.device_wires is not None and wire < len(self.device_wires):
            old_roms = self.device_wires[wire]['roms']
            self.device_wires[wire]['roms'] = roms
            self.wire_roms_changed.emit(wire, old_roms, roms)

        self.mcu_roms.emit(wire, r[1:4], roms)
        return roms, list(r[1:4])

    # returns temperature values from specified wire if no errors met
    def get_temp(self, wire):
        r = list(self.mcu_write_read(self.MCUCommands.GET_TEMP.value, [], wire=wire))
        
        if r[1] < self.device_wires[wire]['devices']:
            raise ValueError('Количество датчиков в ответе отлично от ожидаемого: %d вместо %d, линия %d' % (r[1], self.device_wires[wire]['devices'], wire+1))

        temp = []
        for i in range(r[1]):
            t = (r[4+i*2] | r[4+i*2+1] << 8) / 16.0
            if t < -60 or t > 130:
                raise ValueError('Значение температуры вышло из допустимого диапазона (%f)' % t)
            temp.append(t)
            
        # update an information about wires inside the interface class object
        self.update_wire_info(wire, r[1:4])

        self.mcu_temp.emit(wire, r[1:4], temp)
        return temp

    # returns device memory bytes from specified wire if no errors met
    def get_device_memory(self, wire):
        r = list(self.mcu_write_read(self.MCUCommands.GET_DMEM.value, [], wire=wire))

        if r[1] < self.device_wires[wire]['devices']:
            raise ValueError('Количество датчиков в ответе отлично от ожидаемого: %d вместо %d, линия %d' % (r[1], self.device_wires[wire]['devices'], wire+1))
            
        dmem = []
        for i in range(r[1]):
            m = []
            for j in range(5):
                m.append(r[4 + i * 5 + j])
            dmem.append(m)
                                    
        # update an information about wires inside the interface class object
        self.update_wire_info(wire, r[1:4])

        self.mcu_device_memory.emit(wire, r[1:4], dmem)
        return dmem

    def scan_roms_on_wire(self, wire):
        self.mcu_write(self.MCUCommands.SCAN_WIRE.value, [], wire=wire)

    def set_wire_status(self, parameters):
        # parameters = [wire, status]
        self.mcu_write(self.MCUCommands.SET_WSTS.value, [parameters[1]], wire=parameters[0])

    def set_resolution(self, resolution):
        self.mcu_write(self.MCUCommands.SET_RESL.value, [resolution.value])

    def get_bmx_data(self):
        r = list(self.mcu_write_read(self.MCUCommands.GET_BMx.value, [])[1:13])

        T = struct.unpack('f', bytes(r[0:4]))[0]
        P = struct.unpack('f', bytes(r[4:8]))[0]
        res = [T, P]

        if self.bmx_state & self.BMx280States.BME_DEVICE.value:
            H = struct.unpack('f', bytes(r[8:12]))[0]
            res.append(H)

        self.mcu_bmx_data.emit(res)
        return res

    # interface attributes
    def request_mcu_status(self):
        self.queue.put_nowait([self.get_mcu_status])

    def request_get_time(self):
        self.queue.put_nowait([self.get_time])

    def request_last_info(self):
        self.queue.put_nowait([self.get_last_info])

    def request_roms(self, wire):
        self.queue.put_nowait([self.get_roms, wire])

    def request_temp(self, wire):
        self.queue.put_nowait([self.get_temp, wire])

    def request_device_memory(self, wire):
        self.queue.put_nowait([self.get_device_memory, wire])

    def request_mode(self, mode):
        self.queue.put_nowait([self.set_mode, mode])

    def request_set_time(self):
        self.queue.put_nowait([self.set_time])

    def request_set_period(self, period):
        self.queue.put_nowait([self.set_period, period])

    def request_rescan_wire(self, wire):
        self.queue.put_nowait([self.scan_roms_on_wire, wire])

    def request_new_wire_status(self, parameters):
        self.queue.put_nowait([self.set_wire_status, parameters])

    def request_set_resolution(self, resolution):
        self.queue.put_nowait([self.set_resolution, resolution])

    def request_bmx_data(self):
        self.queue.put_nowait([self.get_bmx_data])

    def device_info_receive(self):
        self.msleep(100)
        try:
            status = self.get_mcu_status()
            mode = status[1]
            period = status[3]
            resolution = status[4]
            wires_count = status[5]
            bmx_state = status[6]

            mode_check = self.MCUModes(mode)
            resolution_check = self.MCUResolutions(resolution)

            wires = []

            for w in range(wires_count):
                wires.append({})
                wire_roms, wire_info = self.get_roms(w)
                wires[w]['devices'] = wire_info[0]
                wires[w]['active'] = wire_info[1]
                wires[w]['roms'] = wire_roms
                wires[w]['status'] = 0

            self.device_mode = mode
            self.device_period = period
            self.device_resolution = resolution
            self.device_wires = wires
            self.bmx_state = bmx_state
            self.info_received.emit(wires, mode, period, resolution, bmx_state)
        except:
            self.__device_connected = False
            self.send_status.emit('Не удается получить информацию об устройстве. Отключение', 1000)

    # returns DS state as string
    def DSState2str(self, state):
        if state in self.DSStates.keys():
            return self.DSStates[state]
        
        res_str = ''
        for k in self.DSStates.keys():
            if state & k:
                if len(res_str) != 0:
                    res_str += ', '
                    
                res_str += self.DSStates[k]
        
        return res_str

    # thread loop
    def run(self):
        self.sleep(1)
        self.send_status.emit('Запуск интерфейса МК ...', 100)

        self.sleep(1)

        while self.thread_active:
            device = usb.core.find(idVendor=self.VID, idProduct=self.PID, backend=self.backend)

            if device is None:
                self.__device_connected = False

            while self.thread_active and not self.__device_connected:
                try:
                    if self.mcu_connect():
                        self.send_status.emit('Устройство найдено!', 1000)
                        self.device_connected.emit()                
                    else:
                        while not self.queue.empty():
                            task = self.queue.get_nowait()

                        self.device_disconnected.emit()
                        self.send_status.emit('Устройство не найдено!', 0)

                except usb.core.USBError as e:
                    if 'busy' in e.strerror:
                        self.send_status.emit('Устройство уже используется!', 1000)
                except Exception as e:
                    self.send_status.emit('Неизвестная ошибка при попытке подключения к устройству!', 1000)
                    print(e)

                self.msleep(500)

            # here device is connected and ready to work
            try:
                task = self.queue.get(timeout=1)
                if len(task) == 1:
                    task[0]()
                else:
                    task[0](task[1])

            except queue.Empty as e:
                # it is a normal sutuation, continue working
                pass

            except ValueError as e:
                self.send_status.emit(str(e), 2000)

            except usb.core.USBError as e:
                # USB error occured
                if 'time' in str(e.strerror):
                    # timeout error
                    self.send_status.emit('Время ожидания ответа от устройста истекло', 1000)
                else:
                    # unknown usb error
                    print(e.strerror)
            # unexpected error
            except:
                self.send_status.emit('Ошибка при получении данных от устройства', 2000)


# settings window
class SettingsWin(QtWidgets.QDialog):
    def __init__(self, parent=None):
        # ---- CUT! ----

    @QtCore.pyqtSlot()
    def setup_time(self):
        # ---- CUT! ----

    # slot for "save settings" button
    @QtCore.pyqtSlot()
    def save_settings(self):
        # ---- CUT! ----

    # update layout
    @QtCore.pyqtSlot(int, list, list)
    def update_settings_list(self, wire=None, wire_info=None, roms=None):
        # ---- CUT! ----

    # send command to MCU to scan wire
    @QtCore.pyqtSlot(int)
    def scan_wire(self, wire):
        # ---- CUT! ----

    # time received from MCU, update text string
    @QtCore.pyqtSlot(list)
    def time_updated(self, time_list):
       # ---- CUT! ----
    
    # close Event disconnects signals
    def closeEvent(self, event):
        # ---- CUT! ----
		

# main window
class DataLogger(QtWidgets.QMainWindow):
    # main timer variables
    gt_timer_started = False
    get_temp_timer = None

    # arrays to keep tempearture and time values
    temp = []
    time = []

    # variables for GUI
    OK_Pixmap = None
    W_Pixmap = None
    E_Pixmap = None
    StatusPixmapSize = 20
    PlotPoints = 120

    col = ['%s%s%s' % (color, marker, line) for color in ['r', 'g', 'b', 'c', 'm', 'y', 'k'] for marker in ['.', 'P', 'x', '|', '*'] for line in ['-', '--', ':']]    # 5 * 5 * 3 = 75 
    labels = {}
    draw_states = {}

    wires = None
    mode = None
    period = None
    resolution = None
    last_status = None

    bmx_state = None
    bmx_used = False
    bmx_data = None
    bmx_plots = {'temperature': False, 'pressure': False, 'humidity': False}

    wire_boxes = None
    shuffle_button = None

    # settings window object
    settings_win = None

    # signals
    need_draw = QtCore.pyqtSignal()
    settings = QtCore.QSettings('settings.ini', QtCore.QSettings.IniFormat)

    def __init__(self, parent=None):
        QtWidgets.QMainWindow.__init__(self, parent)

        self.load_settings()        
        
        random.shuffle(self.col)       
        
        # preload all dynamic pixmaps
        self.OK_Pixmap = QtGui.QPixmap('icons/status_ok.png')
        self.W_Pixmap = QtGui.QPixmap('icons/status_warn.png')
        self.E_Pixmap = QtGui.QPixmap('icons/status_err.png')

        # main win configuration
        self.setWindowIcon(QtGui.QIcon('icons/programm.png'))
        self.setWindowTitle('Регистратор температуры')
        self.setMinimumSize(750, 500)

        # pixmap for DS state
        self.DSState = QtWidgets.QLabel()
        self.DSState.setPixmap(self.OK_Pixmap.scaled(self.StatusPixmapSize, self.StatusPixmapSize, QtCore.Qt.KeepAspectRatio))
        # self.DSState.setToolTip('DS State')

        # pixmap for FS state
        self.FSState = QtWidgets.QLabel()
        self.FSState.setPixmap(self.OK_Pixmap.scaled(self.StatusPixmapSize, self.StatusPixmapSize, QtCore.Qt.KeepAspectRatio))
        # self.FSState.setToolTip('FS State')

        # widget with DS and FS states
        statesWidget = QtWidgets.QWidget()
        hlayout = QtWidgets.QHBoxLayout(statesWidget)
        hlayout.addWidget(self.DSState)
        hlayout.addWidget(self.FSState)
        hlayout.setContentsMargins(0, 0, 0, 0)

        # status bar configuration
        self.status_bar = QtWidgets.QStatusBar()
        self.status_bar.addPermanentWidget(statesWidget)
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage('Загрузка программы ...')

        # layout configuration
        # setting up central widget
        self.central_widget = QtWidgets.QWidget()
        central_layout = QtWidgets.QVBoxLayout(self.central_widget)
        self.setCentralWidget(self.central_widget)

        # ToolBox on top of the central widget
        self.top_buttons = QtWidgets.QToolBar()
        central_layout.addWidget(self.top_buttons)

        # ToolBox actions
        # action 1: open settings window
        self.settings_action = self.top_buttons.addAction(QtGui.QIcon('icons/settings.png'), 'Настройки')
        self.settings_action.setEnabled(False)
        self.settings_action.triggered.connect(self.settings_action_called)

        # action 2: toggle fast scan mode
        self.fast_mode_action = self.top_buttons.addAction(QtGui.QIcon('icons/mode.png'), 'Режим быстрого сканирования')
        self.fast_mode_action.setCheckable(True)
        self.fast_mode_action.setEnabled(False)
        self.fast_mode_action.triggered.connect(self.fast_mode_triggered)

        # action 3: make a snapshot action
        self.save_plot = self.top_buttons.addAction(QtGui.QIcon('icons/snapshot.png'), 'Сохранить график в файл')
        self.save_plot.setEnabled(False)
        self.save_plot.triggered.connect(self.make_snapshot)

        # action 4: replace roms in specified csv files
        self.replace_roms = self.top_buttons.addAction(QtGui.QIcon('icons/convert.png'), 'Заменить в CSV файлах ROM коды на метки')
        # self.replace_roms.setEnabled(False)
        self.replace_roms.triggered.connect(self.replace_roms_in_csv)

        # central area with graph and deviecs list
        self.box_widget = QtWidgets.QWidget()
        box_layout = QtWidgets.QHBoxLayout(self.box_widget)
        central_layout.addWidget(self.box_widget)

        # Graph area: two possible states 1) QLabel, 2) matplotlib.canvas
        self.graph_area = QtWidgets.QStackedWidget()

        # splash label
        self.splash = QtWidgets.QLabel('Подключите устройство для продолжения работы')
        self.splash.setAlignment(QtCore.Qt.AlignCenter)
        self.splash.setStyleSheet('border: 1px solid #cccccc')

        # graph area
        self.graph = Figure()
        self.graph.autofmt_xdate()
        self.axis = self.graph.add_subplot(111)
        self.canvas = FigureCanvas(self.graph)
        self.graph_area.addWidget(self.splash)
        self.graph_area.addWidget(self.canvas)
        box_layout.addWidget(self.graph_area)
        self.graph.subplots_adjust(bottom=0.07, top=0.94, right=0.97)

        # dewices list
        self.list_scroll = QtWidgets.QScrollArea()
        self.list_widget = QtWidgets.QWidget()
        self.list_scroll.setFixedWidth(180)
        self.list_scroll.setWidgetResizable(True)
        self.list_scroll.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.list_scroll.setWidget(self.list_widget)
        box_layout.addWidget(self.list_scroll)

        # logic initialization
        self.mcu_interface = MCUInterface()

        # basic connections between signals and slots
        # need_draw -> redraw
        self.need_draw.connect(self.redraw)
        # MCUInterface.signal -> self.slot
        # status
        self.mcu_interface.send_status.connect(self.on_status_change)
        # update device last information
        self.mcu_interface.mcu_last_info.connect(self.on_mcu_last_info_received)
        # receive temperature values
        self.mcu_interface.mcu_temp.connect(self.on_mcu_temp_received)
        # setting up the interface after device information received
        self.mcu_interface.info_received.connect(self.on_mcu_info_received)
        # freeze interface when device disconnected
        self.mcu_interface.device_disconnected.connect(self.on_device_disconnected)

        # for bmx tests
        self.mcu_interface.mcu_bmx_data.connect(self.on_bmx_data_received)

        # MCUInterface thread start
        self.mcu_interface.start()

    # serves for loading some settings from ini file
    def load_settings(self):
        # now we have nothing to load
        pass

    # serves fro requering data from MCU        
    def request_data(self):
        # request temperature values from active wires with devices
        for w in range(len(self.wires)):
            if self.wires[w]['active'] and self.wires[w]['devices'] > 0:
                self.mcu_interface.request_temp(w)
        
        # request device work conditions
        self.mcu_interface.request_last_info()

        # for bmx test
        if self.bmx_used:
            self.mcu_interface.request_bmx_data()
        
    # timer routine
    def timerEvent(self, event):
        self.request_data()

    # shows message in status bar
    @QtCore.pyqtSlot(str, int)
    def on_status_change(self, new_status, timeout=0):
        self.status_bar.clearMessage()
        self.status_bar.showMessage(new_status, timeout)

    # performs initializations after device have been connected and configured
    @QtCore.pyqtSlot(list, int, int, int, int)
    def on_mcu_info_received(self, wires, mode, period, resolution, bmx_state):
        self.wires = wires
        self.mode = self.mcu_interface.MCUModes(mode)
        self.resolution = self.mcu_interface.MCUResolutions(resolution)
        self.period = period
        self.bmx_state = bmx_state

        if bmx_state & self.mcu_interface.BMx280States.USE_BMx_SENSOR.value:
            self.bmx_used = True

            self.bmx_data = {}
            self.bmx_data['time'] = []
            self.bmx_data['temperature'] = []
            self.bmx_data['pressure'] = []

            if bmx_state & self.mcu_interface.BMx280States.BME_DEVICE.value:
                self.bmx_data['humidity'] = []
        else:
            self.bmx_used = False

        # clean lists because situations is not defined, need to operate like from the start
        self.labels = {}
        self.draw_states = {}

        self.init_work_mode()

        for w in range(len(self.wires)):
            for d in range(self.wires[w]['devices']):
                rom = self.wires[w]['roms'][d]
                self.labels[rom] = self.settings.value('Labels/%s' % rom, rom)
                self.draw_states[rom] = int(self.settings.value('DrawStates/%s' % rom, 0))

        self.update_wires_widget()
        self.settings_action.setEnabled(True)
        self.fast_mode_action.setEnabled(True)
        self.save_plot.setEnabled(True)

    # Initiates work on mode change
    def init_work_mode(self):
        timer_period = self.period * 60

        if self.mode == self.mcu_interface.MCUModes.FAST_SCAN:
            timer_period = 1
            self.fast_mode_action.setChecked(True)
        else:
            self.fast_mode_action.setChecked(False)

        self.temp = []
        self.time = []

        for w in range(len(self.wires)):
            self.temp.append([])
            self.time.append([])

            for d in range(self.wires[w]['devices']):
                self.temp[w].append([])

        # prerequest device info and temp from active wires
        self.request_data()

        # bmx
        if self.bmx_used:
            self.bmx_data = {}
            self.bmx_data['time'] = []
            self.bmx_data['temperature'] = []
            self.bmx_data['pressure'] = []

            if self.bmx_state & self.mcu_interface.BMx280States.BME_DEVICE.value:
                self.bmx_data['humidity'] = []

        # start timer, prepare graph area for drawing
        self.get_temp_timer = self.startTimer(1000 * timer_period, timerType=QtCore.Qt.VeryCoarseTimer)
        self.gt_timer_started = True
        self.axis.clear()
        self.graph_area.setCurrentIndex(1)

    # serves to update list of devices
    def update_wires_widget(self):
        # widget to be set in scrollArea
        self.list_widget = QtWidgets.QWidget()
        vlayout = QtWidgets.QVBoxLayout(self.list_widget)
        # top label in this widget
        top_label = QtWidgets.QLabel('<b>Вкл/Выкл график</b>')
        vlayout.addWidget(top_label, alignment=QtCore.Qt.AlignCenter)

        self.wire_boxes = []

        for w in range(len(self.wires)):
            group = QtWidgets.QGroupBox('Линия № %d (0)' % (w+1))
            vlayout.addWidget(group)
            group_layout = QtWidgets.QVBoxLayout(group)
            self.wire_boxes.append(group)

            for d in range(len(self.wires[w]['roms'])):
                rom = self.wires[w]['roms'][d]

                device_checkbox = QtWidgets.QCheckBox(self.labels[rom])
                device_checkbox.setCheckState(self.draw_states[rom])
                device_checkbox.setTristate(False)
                # connect checkbox toggle action to on_draw_state_changed function
                device_checkbox.stateChanged.connect(partial(self.on_draw_state_changed, [device_checkbox, rom]))

                # disable operation with checkbox if wire state is disabled
                if not self.wires[w]['active']:
                    device_checkbox.setDisabled(True)

                group_layout.addWidget(device_checkbox)

            if not self.wires[w]['active']:
                group.hide()

        # BMx280
        if self.bmx_used:
            label = 'BMP280'
            if self.bmx_state & self.mcu_interface.BMx280States.BME_DEVICE.value:
                label = 'BME280'
            group = QtWidgets.QGroupBox('Датчик BMx280')
            vlayout.addWidget(group)
            group_layout = QtWidgets.QVBoxLayout(group)
            self.wire_boxes.append(group)

            device_checkbox = QtWidgets.QCheckBox('Температура')
            device_checkbox.setTristate(False)
            device_checkbox.stateChanged.connect(partial(self.on_bmx_draw_state_changed, [device_checkbox, 'temperature']))
            group_layout.addWidget(device_checkbox)

            device_checkbox = QtWidgets.QCheckBox('Давление')
            device_checkbox.setTristate(False)
            device_checkbox.stateChanged.connect(partial(self.on_bmx_draw_state_changed, [device_checkbox, 'pressure']))
            group_layout.addWidget(device_checkbox)

            if self.bmx_state & self.mcu_interface.BMx280States.BME_DEVICE.value:
                device_checkbox = QtWidgets.QCheckBox('Влажность')
                device_checkbox.setTristate(False)
                device_checkbox.stateChanged.connect(partial(self.on_bmx_draw_state_changed, [device_checkbox, 'humidity']))
                group_layout.addWidget(device_checkbox)
        
        self.shuffle_button = QtWidgets.QPushButton('Перемешать стили')
        self.shuffle_button.clicked.connect(self.shuffle_styles)
        
        vlayout.addWidget(self.shuffle_button)
        vlayout.addStretch(1)

        self.list_scroll.setWidget(self.list_widget)
        self.list_scroll.setWidgetResizable(True)
        self.list_widget.show()

    # serves for shuffling all draw styles
    @QtCore.pyqtSlot()
    def shuffle_styles(self):
        random.shuffle(self.col)
        self.need_draw.emit()

    # serves for correct device disconnection
    @QtCore.pyqtSlot()
    def on_device_disconnected(self):
        self.graph_area.setCurrentIndex(0)
        self.settings_action.setEnabled(False)
        self.fast_mode_action.setEnabled(False)
        self.save_plot.setEnabled(False)

        if self.gt_timer_started:
            self.killTimer(self.get_temp_timer)
            self.gt_timer_started = False

        self.list_scroll.setWidget(QtWidgets.QWidget())

    # update condition labels in GUI
    @QtCore.pyqtSlot(list)
    def on_mcu_last_info_received(self, info_list):
        self.last_status = info_list

        # lastFSState
        fs_pixmap = self.OK_Pixmap
        fs_tooltip = 'Запись на SD карту: %s' % self.mcu_interface.FSStates[info_list[1]]
        if info_list[1] != 0:
            fs_pixmap = self.W_Pixmap

        # lastDSState
        ds_pixmap = self.OK_Pixmap
        ds_tooltip = 'Опрос линий: %s' % self.mcu_interface.DSState2str(info_list[2])
        if info_list[2] != 0:
            ds_pixmap = self.W_Pixmap

        self.DSState.setPixmap(
            ds_pixmap.scaled(self.StatusPixmapSize, self.StatusPixmapSize, QtCore.Qt.KeepAspectRatio))
        self.DSState.setToolTip(ds_tooltip)
        self.FSState.setPixmap(
            fs_pixmap.scaled(self.StatusPixmapSize, self.StatusPixmapSize, QtCore.Qt.KeepAspectRatio))
        self.FSState.setToolTip(fs_tooltip)

    # slot for checkboxes from wires area
    @QtCore.pyqtSlot(list)
    def on_draw_state_changed(self, par_list):
        checkbox = par_list[0]
        rom = par_list[1]

        if checkbox.checkState() == 0:
            self.draw_states[rom] = 0
        else:
            self.draw_states[rom] = 1
        self.settings.setValue('DrawStates/%s' % rom, self.draw_states[rom])
        self.need_draw.emit()

    # slot for bmx checkboxes
    @QtCore.pyqtSlot(list)
    def on_bmx_draw_state_changed(self, par_list):
        checkbox = par_list[0]
        key = par_list[1]

        if checkbox.checkState() == 0:
            self.bmx_plots[key] = False
        else:
            self.bmx_plots[key] = True
        self.need_draw.emit()

    # performs operation of saving temperature and corresponding time
    @QtCore.pyqtSlot(int, list, list)
    def on_mcu_temp_received(self, wire, wire_info, temp):
        # save wire properties
        self.wires[wire]['active'] = wire_info[1]
        self.wires[wire]['status'] = wire_info[2]

        self.wire_boxes[wire].setTitle('Линия № %d (%d)' % (wire+1, wire_info[2]))
        self.wire_boxes[wire].setToolTip(self.mcu_interface.DSState2str(wire_info[2]))

        # do we need to save temp values into self.temp?
        save_time = True

        for d in range(len(temp)):
            if len(self.temp) < wire+1 or len(self.temp[wire]) < d+1:
                # arrays is not ready for now - do not need to save this values
                save_time = False
                continue

            self.temp[wire][d].append(temp[d])

            if len(self.temp[wire][d]) > self.PlotPoints:
                self.temp[wire][d].pop(0)

        if save_time:
            self.time[wire].append(datetime.now())

        # and save time point to draw then
        if len(self.time[wire]) > self.PlotPoints:
            self.time[wire].pop(0)

        self.need_draw.emit()

    @QtCore.pyqtSlot(list)
    def on_bmx_data_received(self, data):
        self.bmx_data['time'].append(datetime.now())
        self.bmx_data['temperature'].append(data[0])
        self.bmx_data['pressure'].append(data[1] / 133.3)

        if self.bmx_state & self.mcu_interface.BMx280States.BME_DEVICE.value:
            self.bmx_data['humidity'].append(data[2])

        if len(self.bmx_data['time']) > self.PlotPoints:
            self.bmx_data['time'].pop(0)
            self.bmx_data['temperature'].pop(0)
            self.bmx_data['pressure'].pop(0)

            if self.bmx_state & self.mcu_interface.BMx280States.BME_DEVICE.value:
                self.bmx_data['humidity'].pop(0)

    # ------------------------------------------------------

    # function (slot) for redrawing plot
    @QtCore.pyqtSlot()
    def redraw(self):
        self.axis.clear()
        self.axis.xaxis_date()

        if self.mode == self.mcu_interface.MCUModes.FAST_SCAN:
            self.axis.set_title('Режим быстрого сканирования')
            fmt = dates.DateFormatter('%M:%S')
        else:
            self.axis.set_title('Режим нормального сканирования, период: %d мин' % self.period)
            fmt = dates.DateFormatter('%H:%M')

        color_idx = -1

        lines_on_graph = []
        if self.bmx_plots['pressure']:
            self.axis.set_ylabel('Давление, мм.рт.ст.')

            t, = self.axis.plot(self.bmx_data['time'], self.bmx_data['pressure'], self.col[-1], label='BMx280 давление')
            lines_on_graph.append(t)

        elif self.bmx_plots['humidity']:
            self.axis.set_ylabel('Влажность, %')

            t, = self.axis.plot(self.bmx_data['time'], self.bmx_data['humidity'], self.col[-1], label='BMx280 влажность')
            lines_on_graph.append(t)

        else:
            for w in range(len(self.wires)):
                for d in range(len(self.temp[w])):
                    color_idx += 1
                    rom = self.wires[w]['roms'][d]
                    if self.draw_states[rom] is not 0:
                        t, = self.axis.plot(self.time[w], self.temp[w][d], self.col[color_idx], label=self.labels[rom], markevery=10, alpha=0.7)
                        lines_on_graph.append(t)

            if self.bmx_plots['temperature']:
                t, = self.axis.plot(self.bmx_data['time'], self.bmx_data['temperature'], self.col[-1], label='BMx280 температура', markevery=10, alpha=0.7)
                lines_on_graph.append(t)

            self.axis.set_ylabel('Температура, $^\circ$С')

        self.axis.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
        self.axis.xaxis.set_major_formatter(fmt)
        self.axis.xaxis.set_major_locator(MaxNLocator(7))
        self.axis.grid(b=True, which='both', color='#cccccc', linestyle='--')

        self.axis.legend(handles=lines_on_graph, loc=2)
        self.canvas.draw()

    # ----------------- action slot routines -----------------
    # slot for settings button
    @QtCore.pyqtSlot()
    def settings_action_called(self):
        if self.settings_win is None:
            self.settings_win = SettingsWin(self)
            self.settings_win.show()
            self.settings_win.finished.connect(self.settings_closed)

    # slot for SettingsWin.closed signal
    @QtCore.pyqtSlot()
    def settings_closed(self):
        self.settings_win = None

    # slot which performs changing of mode
    @QtCore.pyqtSlot()
    def fast_mode_triggered(self):
        if self.fast_mode_action.isChecked():
            self.mcu_interface.request_mode(self.mcu_interface.MCUModes.FAST_SCAN)
            self.mode = self.mcu_interface.MCUModes.FAST_SCAN
        else:
            self.mcu_interface.request_mode(self.mcu_interface.MCUModes.NORMAL_SCAN)
            self.mode = self.mcu_interface.MCUModes.NORMAL_SCAN

        if self.gt_timer_started:
            self.killTimer(self.get_temp_timer)
            self.gt_timer_started = False

        self.init_work_mode()

    # snapshot action handler
    def make_snapshot(self):
        filename = 'snapshot_%s.png' % datetime.strftime(datetime.now(), '%Y%m%d_%H%M%S')

        self.graph.savefig(filename, dpi=300)
        self.on_status_change('График сохранен в файл %s' % filename, 2000)

    # close settings window before closing main
    def closeEvent(self, event):
        if self.settings_win is not None:
            self.settings_win.close()

    # replace labels action handler
    @QtCore.pyqtSlot()
    def replace_roms_in_csv(self):
        try:
            self.settings.beginGroup('Labels')
            roms_with_labels = self.settings.allKeys()
            self.settings.endGroup()

            choosen_files = QtWidgets.QFileDialog.getOpenFileNames(self, 'Выбор файлов для обработки', '', 'CSV (*.csv)')
            for fname in choosen_files[0]:
                file = open(fname)
                file_content = file.read()
                file.close()
                
                file_content = file_content.replace('.', ',')

                for rom in roms_with_labels:
                    label = self.settings.value('Labels/%s' % rom)
                    file_content = file_content.replace('R' + rom, label)

                file = open(fname.replace('.CSV', '_labels.CSV'), 'w')
                file.write(file_content)
                file.close()

            if len(choosen_files) > 0:
                self.on_status_change('ROM коды в файлах успешно заменены на метки', 2000)
            else:
                self.on_status_change('Операция завершена, файлы не изменены', 2000)
        except:
            self.on_status_change('В процессе замены ROM кодов на метки в файлах возникла ошибка!', 2000)
			

# for running the application
def run_app():
    app = QtWidgets.QApplication(sys.argv)

    win = DataLogger()
    win.show()

    sys.exit(app.exec_())    


# for executing this file
if __name__ == '__main__':
    run_app()
