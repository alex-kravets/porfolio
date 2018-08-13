#ifndef FISIKA_H
#define FISIKA_H

#include <QWidget>
#include <QTabWidget>
#include <QLine>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QToolButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QGroupBox>

#include <QBoxLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QMessageBox>

#include <QPainter>
#include <QPixmap>
#include <qmath.h>

#include <sstream>

#include <QTimer>

#include <QTextEdit>

//=============================================

class Parameters {
public:
	double h0;
	double v0;
	double an;
	double g;

	double t1;
	double t2;
	double ts;
	double hm;
	double lm;

	QString color;
	QString info;

	Parameters();
	Parameters(double, double, double, double);

	bool operator ==(Parameters&);
	bool operator !=(Parameters&);

	QString getRandColor();
};

class Fisika : public QWidget
{
	Q_OBJECT

public:
	explicit Fisika(QWidget *parent = 0);

private:
	QTabWidget* w_top; //виджет главных вкладок
	QLine* ln_space; //разделитель ?????
	QWidget* w_bottom; //виджет нижний для кнопки выход и др.

	QPushButton* pb_exit; //кнопка выхода
	QPushButton* pb_help; //кнопка помощь

	QWidget* w_workTab; //виджет первой вкладки для графика, параметров ...
	QWidget* w_parmTab; //виджет расчетов, вкладка
	QWidget* w_helpTab; //виджет помощи, вкладка
	QLabel* l_helpTab; //текст помощи

	QGroupBox* gb_startParm; //контейнер параметров
	QLabel* l_height; //метка начальной высоты
	QLabel* l_velocity; //метка начальной скорости
	QLabel* l_angle; //метка начального углa
	QLineEdit* le_height; //поле ввода высоты
	QLineEdit* le_velocity; //поле ввода скорости
	QLineEdit* le_angle; // поле ввода угла
	QLabel* l_g; //метка для выбора g
	QComboBox* cmb_g; //выбор g
	//QLabel* l_seen; //метка наблюдатель
	/*QLabel* l_nHei;
	QLabel* l_nVel;
	QLabel* l_nAng;
	QLabel* l_nA;
	QLineEdit* le_nHei;
	QLineEdit* le_nVel;
	QLineEdit* le_nAng;
	QLineEdit* le_nA;*/
	QGroupBox* gb_more; //дополнительные параметры
	QPushButton* pb_reset; //сброс графиков, результатов
	QPushButton* pb_start; //запуск рисования и расчетов

	QGroupBox* gb_graphArea; //обьединение эдементов графика и анимации
	QLabel* l_graphArea; //виджет для рисования ?????
	bool b_resizeGraph;
	QGroupBox* gb_animCtl; //контроль над анимацией

	QWidget* w_animSetup; //управление анимацией ?????
	QToolButton* tb_animStepPrev; //анимация шаг назад
	QToolButton* tb_animStepNext; //анимация шаг вперед
	//QToolButton* tb_animPlay; //анимация запуск [DELETED]
	QToolButton* tb_animStop; //анимация останов
	QToolButton* tb_animPause; //анимация пауза
	QLabel* l_animTime; //время анимации
	QDoubleSpinBox* spb_timeK; //временной коэффициент
	QTimer* animTimer;
	QSlider* sl_animPos; //управление позицией анимации

	QGroupBox* gb_graphList;
	QGroupBox* gb_graphInfo;

	QLabel* l_graphList;
	QTextEdit* te_graphInfo;

	QLabel* l_helpLabel;
	QPushButton* pb_aboutQt;

	QVector<Parameters> parmVect;

	virtual void resizeEvent(QResizeEvent *); //событие перерисовки графика при изменении размеров окна

	double animPos;

	bool animTrue;

	QPixmap* p_start;
	QPixmap* p_stop;
	QPixmap* p_prev;
	QPixmap* p_next;
	QPixmap* p_pause;

	QIcon* i_start;
	QIcon* i_stop;
	QIcon* i_prev;
	QIcon* i_next;
	QIcon* i_pause;

public slots:
	void viewHelp(); //отображение помощи
	void clear(); //сброс по нажатию на кнопку сброс
	void drawGraph(); //отрисовка графика
	void getParm(); //получение параметров
	double addAnimPos(double); //прибавление позиции анимации для анимации и различных шагов
	double setSliderAnimPos(int); //
	void animation(); //воспроизведение анимации
	void stepPrev(); //шаг назад
	void stepNext(); //шаг вперед
	void stop(); //полная остановка
	void pause(); //пауза таймера
	void aboutQt();
	void viewGraphInfo(QString);

signals:
	void graphDrawed(int pos);
};

#endif // FISIKA_H
