#include "fisika.h"

//#include <QDebug>

Parameters::Parameters() : h0(0), v0(0), an(0), g(0), t1(0), t2(0), ts(0), hm(0), lm(0), color(getRandColor()), info("") {}

Parameters::Parameters(double h, double v, double a, double g1 = 9.8) : h0(h), v0(v), an(a), g(g1), t1(0), t2(0), ts(0), hm(0), lm(0), color(getRandColor()) {
	double alpha = 3.14 / 180 * an;
	hm = (an < 0 || an > 180) ? h0 : (h0 + pow(v0 * sin(alpha), 2) / (2 * g));
	t1 = v0 * sin(alpha) / g;
	t1 = t1 > 0 ? t1 : 0;
	t2 = sqrt(2 * hm / g);
	ts = t1 + t2;
	lm = v0 * cos(alpha) * ts;
}

bool Parameters::operator ==(Parameters& p) {
	return (h0 == p.h0 && v0 == p.v0 && an == p.an && g == p.g);
}

bool Parameters::operator !=(Parameters& p) {
	return !(*this == p);
}

QString Parameters::getRandColor() {
	QString sym("0123456789ABCDEF");
	QString res("#");
	for (int i = 0; i < 6; i++) {
		res += sym[qrand() % 15];
	}
	//qDebug() << res;
	return res;
}

//==================================================================

Fisika::Fisika(QWidget *parent) : QWidget(parent), b_resizeGraph(false), animPos(0), animTrue(true) {

	w_top = new QTabWidget(); //верхний виджет вкладок
	w_bottom = new QWidget(); //нижний виджет для кнопок

	w_workTab = new QWidget(); //вкладка для графика
	w_parmTab = new QWidget(); //вкладка для расчетов
	w_helpTab = new QWidget(); //вкладка помощь

	gb_startParm = new QGroupBox(); //обьединение всех стартовых параметров
	gb_graphArea = new QGroupBox(); //обьединение элементов графика

	pb_exit = new QPushButton("&Выход"); //кнопка выход
	pb_help = new QPushButton("Помощь"); //кнопка помощь

	l_height = new QLabel("Высота <i>H<sub>0</sub>: </i>"); //метка нач. высоты
	l_velocity = new QLabel("Скорость <i>V<sub>0</sub>: </i>"); //метка нач.скорости
	l_angle = new QLabel("Угол <i>&alpha;: </i>"); //метка нач.угла

	le_height = new QLineEdit(); //поле ввода нач. высоты
	le_velocity = new QLineEdit(); //поле ввода нач. скорости
	le_angle = new QLineEdit(); //поле ввода нач.угла

	gb_more = new QGroupBox(); //обьединение дополнительных параметров

	l_g = new QLabel("G планеты:");
	cmb_g = new QComboBox(); //выбор g для разных планет
	QStringList planets; // список планет
	planets << "Меркурий" << "Венера" << "Земля" << "Луна" << "Марс" << "Юпитер" << "Сатурн" << "Уран" << "Нептун" << "Плутон";

	//l_seen = new QLabel("Наблюдатель:");

	/*l_nHei = new QLabel("Высота H:");
	l_nVel = new QLabel("Скорость v:");
	l_nAng = new QLabel("Угол &alpha;:");
	l_nA = new QLabel("Ускорение a:");*/

	/*le_nHei = new QLineEdit();
	le_nVel = new QLineEdit();
	le_nAng = new QLineEdit();
	le_nA = new QLineEdit();*/


	l_graphArea = new QLabel(); //область рисования ???????????????
	gb_animCtl = new QGroupBox(); //обьединени элементов управления анимацией

	tb_animStepPrev = new QToolButton(); //кнопка шаг назад
	tb_animStop = new QToolButton(); //кнопка стоп
	tb_animPause = new QToolButton(); //кнопка пауза
	//tb_animPlay = new QToolButton(); //кнопка запуск
	tb_animStepNext = new QToolButton(); //кнопка шаг вперед
	l_animTime = new QLabel("0"); //время анимации
	spb_timeK = new QDoubleSpinBox(); //временной коэффициент
	sl_animPos = new QSlider(Qt::Horizontal); //слайдер позиции анимации

	pb_reset = new QPushButton("Сброс"); //кнопка сброс
	pb_start = new QPushButton("&Запуск"); //кнопка запуск
	//QIcon i_animPlay("animButtons/play.jpg"); //изображение для кнопки запуск

	animTimer = new QTimer();

	gb_graphList = new QGroupBox();
	gb_graphInfo = new QGroupBox();

	l_helpLabel = new QLabel("<h2>Моделирование движения материальной точки<br> под углом к горизонту</h2><hr><b>Допустимыми значениями являются только положительные числа. Угол задается в пределах [0; 180]</b><br><br>Данная программа предназначена для построения графиков, создания анимации,<br> произведения расчетов на тему 'Движение материальной точки под углом к горизонту'<br>и может использоваться в учебном процессе, как наглядная модель<br>данного процесса, для самообразования а также для расчетов на данную тему.<br><hr><br>Автором данной программы является Кравец Александр.<br>Программа написана на языке С++, с использованием библиотеки Qt<br>является свободнораспространяемой под открытой лицензией GNU LGPL v2.1.<br>");
	pb_aboutQt = new QPushButton("О Qt");

	l_graphList = new QLabel("Расчетные данные<br>отсутствуют.");
	te_graphInfo = new QTextEdit("");

	p_start = new QPixmap(":/animButtons/play.jpg");
	p_stop = new QPixmap(":/animButtons/stop.jpg");
	p_prev = new QPixmap(":/animButtons/stepPrev.jpg");
	p_next = new QPixmap(":/animButtons/stepNext.jpg");
	p_pause = new QPixmap(":/animButtons/pause.jpg");

	i_start = new QIcon(*p_start);
	i_stop = new QIcon(*p_stop);
	i_prev = new QIcon(*p_prev);
	i_next = new QIcon(*p_next);
	i_pause = new QIcon(*p_pause);

	//========================================================== signals
	connect(pb_exit, SIGNAL(clicked()), this, SLOT(close())); //соединение для обработки выхода
	connect(pb_help, SIGNAL(clicked()), this, SLOT(viewHelp())); //соединение для открытия окна помощь
	connect(pb_reset, SIGNAL(clicked()), this, SLOT(clear())); //соединение для сброса

	connect(pb_start, SIGNAL(clicked()), this, SLOT(getParm())); //рисование графика

	connect(tb_animStepPrev, SIGNAL(clicked()), this, SLOT(stepPrev())); //шаг назад
	connect(tb_animStepNext, SIGNAL(clicked()), this, SLOT(stepNext())); //шаг назад
	//connect(tb_animPlay, SIGNAL(clicked()), this, SLOT(animation())); //запуск анимации
	connect(tb_animStop, SIGNAL(clicked()), this, SLOT(stop())); // остановка анимации

	connect(sl_animPos, SIGNAL(valueChanged(int)), this, SLOT(setSliderAnimPos(int)));

	connect(tb_animPause, SIGNAL(clicked()), this, SLOT(pause()));

	connect(pb_aboutQt, SIGNAL(clicked()), this, SLOT(aboutQt()));

	//connect(this, SIGNAL(graphDrawed(int)), sl_animPos, SLOT(setValue(int)));

	connect(l_graphList, SIGNAL(linkActivated(QString)), this, SLOT(viewGraphInfo(QString)));


	//========================================================== construct
	QVBoxLayout* lt_main = new QVBoxLayout(); //основной слой

	QHBoxLayout* lt_bottom = new QHBoxLayout(); //нижний слой
	QHBoxLayout* lt_workTab = new QHBoxLayout(); //слой для вкладки с графиком
	QHBoxLayout* lt_parmTab = new QHBoxLayout();

	QVBoxLayout* lt_startParm = new QVBoxLayout(); //слой для начальных параметров

	QVBoxLayout* lt_lStart = new QVBoxLayout(); //слой меток начальных параметров
	QVBoxLayout* lt_eStart = new QVBoxLayout(); //слой полей ввода начальных параметров
	QHBoxLayout* lt_sumStart = new QHBoxLayout(); //суммарный слой для начальных параметров
	QHBoxLayout* lt_moreParm = new QHBoxLayout(); //слой для дополнительных параметров
	QVBoxLayout* lt_moreParmL = new QVBoxLayout(); //слой для меток дополнительных параметров
	QVBoxLayout* lt_moreParmS = new QVBoxLayout(); //слой для управления дополнительными параметрами

	QVBoxLayout* lt_graphArea = new QVBoxLayout(); //слой для части с графиком

	QHBoxLayout* lt_animCtl = new QHBoxLayout(); //слой для управления анимацией

	QVBoxLayout* lt_helpTab = new QVBoxLayout();

	QVBoxLayout* lt_graphList = new QVBoxLayout();
	QVBoxLayout* lt_graphInfoSa = new QVBoxLayout();


	//========================================================== settings

	l_helpLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

	te_graphInfo->setAlignment(Qt::AlignTop);
	te_graphInfo->setReadOnly(true);

	l_graphList->setAlignment(Qt::AlignTop);

	l_helpLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	l_helpLabel->setStyleSheet("background: white");

	pb_aboutQt->setFixedWidth(100);

	lt_helpTab->setMargin(50);

	gb_graphList->setTitle("   Список графиков   ");
	gb_graphList->setFixedWidth(160);

	gb_graphInfo->setTitle("   Информация о графике   ");

	l_graphArea->setText("После введения корректных данных<br>здесь будет отображен график.<br><br>Результаты расчетов можно найти во вкладке 'Расчеты'.<br><br>Дополнительную помощь можно найти во вкладке 'Помощь'.");
	l_graphArea->setAlignment(Qt::AlignCenter);

	l_animTime->setFixedWidth(80);
	l_animTime->setAlignment(Qt::AlignCenter);

	gb_animCtl->setEnabled(false);

	spb_timeK->setRange(0, 2);
	spb_timeK->setSingleStep(0.1);

	sl_animPos->setRange(0, 1000);
	//sl_animPos->setSingleStep(1);
	sl_animPos->setPageStep(1);
	sl_animPos->setTickInterval(20);
	sl_animPos->setTickPosition(QSlider::TicksBelow);

	tb_animStepPrev->setIcon(*i_prev);
	tb_animStepPrev->setIconSize(QSize(25,25));
	tb_animStepPrev->setStyleSheet("border:none");

	tb_animStepNext->setIcon(*i_next);
	tb_animStepNext->setIconSize(QSize(25,25));
	tb_animStepNext->setStyleSheet("border:none");

	tb_animStop->setIcon(*i_stop);
	tb_animStop->setIconSize(QSize(25,25));
	tb_animStop->setStyleSheet("border:none");

	tb_animStepPrev->setText("наз");
	tb_animStepNext->setText("впер");
	tb_animStop->setText("стоп");
	tb_animPause->setText("зап");

	//tb_animPlay->setIcon(i_animPlay);
	//tb_animPlay->setIconSize(QSize(25,25));
	//tb_animPlay->setStyleSheet("border:none");

	tb_animPause->setIcon(*i_start);
	tb_animPause->setIconSize(QSize(25,25));
	tb_animPause->setStyleSheet("border:none");

	l_graphArea->setStyleSheet("border: 1px solid #ddd");

	lt_graphArea->setSpacing(10);
	lt_graphArea->setMargin(10);

	cmb_g->addItems(planets);
	cmb_g->setCurrentIndex(2);

	lt_moreParm->setSpacing(5);

	gb_more->setTitle(" Дополнительно   ");
	gb_more->setCheckable(true);
	gb_more->setChecked(false);

	lt_sumStart->setSpacing(5);

	lt_startParm->setAlignment(Qt::AlignTop);
	lt_startParm->setSpacing(10);
	//lt_startParm->setMargin();

	gb_startParm->setTitle("   Параметры   ");
	gb_startParm->setFixedWidth(200);

	gb_graphArea->setTitle("   График   ");

	lt_workTab->setMargin(10);
	lt_workTab->setSpacing(10);

	lt_bottom->setMargin(20);

	lt_main->setMargin(5);
	lt_main->setSpacing(10);



	//========================================================== tooltips




	//========================================================== release


	lt_helpTab->addWidget(l_helpLabel);
	lt_helpTab->addWidget(pb_aboutQt);
	w_helpTab->setLayout(lt_helpTab);

	//sa_graphInfo->setWidget(l_graphInfo);

	lt_graphInfoSa->addWidget(te_graphInfo);
	gb_graphInfo->setLayout(lt_graphInfoSa);

	lt_graphList->addWidget(l_graphList);
	gb_graphList->setLayout(lt_graphList);

	lt_parmTab->addWidget(gb_graphList);
	lt_parmTab->addWidget(gb_graphInfo);
	w_parmTab->setLayout(lt_parmTab);

	//work end

	lt_animCtl->addWidget(tb_animStepPrev);
	lt_animCtl->addWidget(tb_animStop);
	lt_animCtl->addWidget(tb_animPause);
	//lt_animCtl->addWidget(tb_animPlay);
	lt_animCtl->addWidget(tb_animStepNext);
	lt_animCtl->addWidget(spb_timeK);
	lt_animCtl->addWidget(sl_animPos, 1);
	lt_animCtl->addWidget(l_animTime);

	gb_animCtl->setLayout(lt_animCtl);

	lt_graphArea->addWidget(l_graphArea, 1);
	lt_graphArea->addWidget(gb_animCtl);

	gb_graphArea->setLayout(lt_graphArea);



	lt_moreParmL->addWidget(l_g);
	lt_moreParmS->addWidget(cmb_g);
	/*lt_moreParmL->addWidget(l_seen);
	lt_moreParmS->addWidget(l_space);
	lt_moreParmL->addWidget(l_nHei);
	lt_moreParmL->addWidget(l_nVel);
	lt_moreParmL->addWidget(l_nAng);
	lt_moreParmL->addWidget(l_nA);
	lt_moreParmS->addWidget(le_nHei);
	lt_moreParmS->addWidget(le_nVel);
	lt_moreParmS->addWidget(le_nAng);
	lt_moreParmS->addWidget(le_nA);*/

	lt_moreParm->addLayout(lt_moreParmL);
	lt_moreParm->addLayout(lt_moreParmS);
	gb_more->setLayout(lt_moreParm);

	lt_lStart->addWidget(l_height);
	lt_lStart->addWidget(l_velocity);
	lt_lStart->addWidget(l_angle);

	lt_eStart->addWidget(le_height);
	lt_eStart->addWidget(le_velocity);
	lt_eStart->addWidget(le_angle);


	lt_sumStart->addLayout(lt_lStart);
	lt_sumStart->addLayout(lt_eStart);

	lt_startParm->addLayout(lt_sumStart);
	lt_startParm->addWidget(gb_more);
	lt_startParm->addStretch(1);
	lt_startParm->addWidget(pb_reset);
	lt_startParm->addWidget(pb_start);

	lt_workTab->addWidget(gb_startParm);
	lt_workTab->addWidget(gb_graphArea, 1);

	w_workTab->setLayout(lt_workTab);

	gb_startParm->setLayout(lt_startParm);

	w_top->addTab(w_workTab, "&График");
	w_top->addTab(w_parmTab, "&Расчеты");
	w_top->addTab(w_helpTab, "&Помощь");

	lt_bottom->addWidget(pb_help);
	lt_bottom->addStretch(1);
	lt_bottom->addWidget(pb_exit);

	w_bottom->setLayout(lt_bottom);

	lt_main->addWidget(w_top);
	lt_main->addWidget(w_bottom);

	setLayout(lt_main);
	setMinimumSize(800, 600);
	setWindowTitle("Моделирование движения материальной точки под углом к горизонту");
}

void Fisika::viewHelp() {
	w_top->setCurrentIndex(2);
}

void Fisika::clear() {
	le_height->setText("");
	le_velocity->setText("");
	le_angle->setText("");
	if (parmVect.size()) parmVect.remove(0, parmVect.size());
	gb_animCtl->setEnabled(false);
	gb_more->setChecked(false);
	l_animTime->setText("0");
	sl_animPos->setValue(0);
	spb_timeK->setValue(0);
	cmb_g->setCurrentIndex(2);
	l_graphList->setText("Расчетные данные<br>отсутствуют.");
	te_graphInfo->setText("");
	animPos = 1;
	drawGraph();
}

void Fisika::drawGraph() {
	if (!b_resizeGraph) b_resizeGraph = true;

	int space[2] = {30, 30}; //[x, y];
	int margins[4] = {40, 40, 40, 60}; //[top, right, bottom, left];
	int netStep = 10;
	int metStep = 60;
	int areaWidth = l_graphArea->width();
	int areaHeight = l_graphArea->height();

	QPixmap pixm(areaWidth, areaHeight);
	pixm.fill(Qt::white);
	QPainter painter(&pixm);
	painter.setPen(QColor("#ccc"));

	if (parmVect.size()) {
		for (int i = 1; i <= areaWidth / netStep; i++) { //vertical net
			painter.drawLine(i * 10, 0, i * 10, areaHeight);
		}
		for (int i = 1; i <= areaHeight / netStep; i++) { //horizontal net
			painter.drawLine(0, i * 10, areaWidth, i * 10);
		}

		painter.setPen(Qt::black);
		painter.drawLine(margins[3] - 10, areaHeight - margins[2], areaWidth - margins[1], areaHeight - margins[2]); // Ox
		painter.drawLine(margins[3], margins[0], margins[3], areaHeight - margins[2] + 10); //Oy
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.drawLine(areaWidth - margins[1] - 20, areaHeight - margins[2] - 3 + 1, areaWidth - margins[1], areaHeight - margins[2] + 1);
		painter.drawLine(areaWidth - margins[1] - 20, areaHeight - margins[2] + 3, areaWidth - margins[1], areaHeight - margins[2]);
		painter.drawLine(margins[3] - 3, margins[0] + 20, margins[3], margins[0]);
		painter.drawLine(margins[3] + 3, margins[0] + 20, margins[3], margins[0]);
		painter.setRenderHint(QPainter::Antialiasing, false);

		// дойдя до этого места мы уже нарисовали оси и сетку

		double mgdh = 0, mgdw = 0, mtd = 0;
		int mln = 0;
		for (int i = 0; i < parmVect.size(); i++) {
			mgdh = (parmVect[i].hm > mgdh) ? parmVect[i].hm : mgdh;
			if (parmVect[i].lm > mgdw) {
				mgdw = parmVect[i].lm;
				mln = i;
			}
			mtd = (parmVect[i].ts > mtd) ? parmVect[i].ts : mtd; //максимальное время полета (для анимации)
		}

		if ((mgdh || mgdw) && mtd) {
			double realx = areaWidth - (margins[3] + margins[1] + space[0]);
			double realy = areaHeight - (margins[0] + margins[0] + space[1]);

			//qDebug() << realx << realy;

			double gk1 = realx / mgdw;
			double gk2 = realy / mgdh;
			double gk = (gk1 < gk2) ? gk1 : gk2;
			double tk = mtd * animPos;

			double textk = (mgdh / realy > mgdw / realx) ? mgdh / realy : mgdw / realx;

			painter.drawText(margins[3] / 2, margins[0] / 2, "H, м");
			painter.drawText(areaWidth - margins[1], areaHeight - margins[2] * 0.5, "L, м");

			painter.translate(margins[3], areaHeight - margins[2]);

			painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
			painter.setRenderHint(QPainter::TextAntialiasing, true);
			painter.setRenderHint(QPainter::Antialiasing, true);


			for (int i = 0; i < parmVect.size(); i++) {
				painter.setPen(parmVect[i].color);
				double px = 0, py = 0;
				for (double j = 0; j < tk; j += mtd / 3000) {
					px = (parmVect[i].v0 * cos(3.1415 / 180 * parmVect[i].an) * j) * gk;
					py = -1 * (parmVect[i].h0 + parmVect[i].v0 * sin(3.1415 / 180 * parmVect[i].an) * j - parmVect[i].g * j * j / 2) * gk;
					QPointF pn(px, py);
					if (py <= 0 && px >= 0) painter.drawPoint(pn);
				}				
				if (py <= 0 && px >= 0) painter.drawArc(px - 2, py - 2, 4, 4, 0, 16 * 360);
			}

			painter.setPen(Qt::black);

			painter.drawText(-15, 15, "0");
			for (int i = 1; i < realx / metStep; i++) {
				std::ostringstream buf;
				buf << floor(i *  metStep * textk * 10) / 10;
				painter.drawText(i * metStep, 15, buf.str().c_str());
			}

			for (int i = 1; i < realy / metStep; i++) {
				std::ostringstream buf;
				buf << floor(i *  metStep * textk * 10) / 10;
				painter.drawText(-60, -i * metStep, 50, 15, Qt::AlignRight, buf.str().c_str());
			}

			int h = mtd * animPos / 3600;
			int m = mtd * animPos / 60 - h * 60;
			double s = mtd * animPos - m * 60 - h * 3600;
			s = floor(s * 1000) / 1000;
			std::ostringstream time;
			time << h << "ч " << m << "м " << s << "с";
			l_animTime->setText(time.str().c_str());

			if (animPos >= 1.0) {
				double lhm = parmVect[mln].h0 * gk,
					   lwm = parmVect[mln].lm * gk,
					   //lhh = parmVect[mln].hm * gk,
					   lan = parmVect[mln].an * 3.14 / 180;

				double px = 0, py = 0;
				px = (parmVect[mln].v0 * cos(3.1415 / 180 * parmVect[mln].an) * parmVect[mln].t1) * gk;
				py = -1 * (parmVect[mln].h0 + parmVect[mln].v0 * sin(3.1415 / 180 * parmVect[mln].an) * parmVect[mln].t1 - parmVect[mln].g * parmVect[mln].t1 * parmVect[mln].t1 / 2) * gk;

				painter.drawLine(lwm, 0, lwm, -70);
				painter.drawText(lwm - 30, -80, QString("Lmax = %1").arg(parmVect[mln].lm));
				painter.drawLine(px, 0, px, py);
				painter.drawText(px + 3, py - 10, QString("Hmax = %1").arg(parmVect[mln].hm));
				painter.translate(0, -lhm);
				if (parmVect[mln].an > 0 && parmVect[mln].an < 180 && parmVect[mln].an != 90) {
				painter.drawLine(0, 0, 90 * cos(lan), 0);
					painter.drawArc(-30, -30, 60, 60, 0 * 16, (parmVect[mln].an) * 16);
					painter.drawText(33, -30 * tan(parmVect[mln].an * 3.1415 / 360), "a");
				}
				painter.drawText(90 * cos(lan),- 90 * sin(lan) - 10, "Vo");
				painter.rotate(-lan * 180 / 3.14);
				painter.drawLine(0, 0, 90, 0);
				painter.drawLine(90, 0, 80, -3);
				painter.drawLine(90, 0, 80, 3);
				painter.drawArc(0, 30, 30, 0, 10, 0);

			}
		}

	}
	l_graphArea->setPixmap(pixm);
	//emit graphDrawed(animPos * 100);
}

void Fisika::resizeEvent(QResizeEvent *) {
	if (b_resizeGraph) drawGraph();
}

void Fisika::getParm() {
	double h0;
	double v0;
	double an;
	double g = 9.8;
	std::string pl = "Земля";
	std::ostringstream info;

	if (le_height->text().contains(QRegExp("\\D")) || le_height->text().isEmpty()) {
		QMessageBox::warning(this, "Неправильный параметр!", "'Высота H<sub>0</sub>: " + le_height->text() + "'<br><br>Не является допустимым значением!<br><br>О допустимых значениях подробно описано во вкладке 'Помощь'.");
		return;
	}
	h0 = le_height->text().toDouble();
	if (le_velocity->text().contains(QRegExp("\\D")) || le_velocity->text().isEmpty()) {
		QMessageBox::warning(this, "Неправильный параметр!", "'Скорость V<sub>0</sub>: " + le_velocity->text() + "'<br><br>Не является допустимым значением!<br><br>О допустимых значениях подробно описано во вкладке 'Помощь'.");
		return;
	}
	v0 = le_velocity->text().toDouble();
	if (le_angle->text().contains(QRegExp("\\D")) || le_angle->text().isEmpty()) {
		QMessageBox::warning(this, "Неправильный параметр!", "'Угол &alpha;: " + le_angle->text() + "'<br><br>Не является допустимым значением!<br><br>О допустимых значениях подробно описано во вкладке 'Помощь'.");
		return;
	}
	an = le_angle->text().toDouble();
	an = an - int(an / 360) * 360;
	if (an > 90 && an <= 180) {
		an = 180 - an;
	}
	if (an > 180) {
		QMessageBox::warning(this, "Неправильный параметр!", "'Угол &alpha;: " + le_angle->text() + "'<br><br>Не является допустимым значением!<br><br>О допустимых значениях подробно описано во вкладке 'Помощь'.");
		return;
	}

	if (gb_more->isChecked()) {
		switch (cmb_g->currentIndex()) { //planets selection
			case 0:
				g = 3.7; //меркурий
				pl = "Меркурий";
				break;
			case 1:
				g = 8.87; //venus
				pl = "Венера";
				break;
			case 2:
				g = 9.8; //earth
				pl = "Земля";
				break;
			case 3:
				g = 1.62; //luna
				pl = "Луна";
				break;
			case 4:
				g = 3.711; //mars
				pl = "Марс";
				break;
			case 5:
				g = 24.79; //jupiter
				pl = "Юпитер";
				break;
			case 6:
				g = 10.44; //saturn
				pl = "Сатурн";
				break;
			case 7:
				g = 8.69; //uranus
				pl = "Уран";
				break;
			case 8:
				g = 11.15; //neptune
				pl = "Нептун";
				break;
			case 9:
				g = 0.58; //pluto;
				pl = "Плутон";
				break;
			default:
			g = 9.79;
			break;
		}
	}


	Parameters graph(h0, v0, an, g);

	info << "<center><h2>Расчет параметров движения материальной точки<br> по углом к горизонту</h2></center><hr><b>Теория:</b><ul><li>Тело, брошенное горизонтально, движется <i>по ветви параболы под действием силы тяжести.</i></li><li>Время падения (полета) тела брошенного горизонтально, равно сумме времени подьема и времени падения.</li><li>Движение тела по оси Oy является равноускоренным (свободным).</li><li>Движение тела по оси Ox является равномерным.</li></ul><br><hr><br><b>Формулы, использованные в ходе расчетов:</b><br><table><tr><td valign='middle'>Расчет максимальной высоты</td><td align='left'><img src=':/formuls/maxh.png'</td></tr><tr><td valign='middle'>Расчет времени подъема</td><td align='left'><img src=':/formuls/tpod.png'></tr><tr><td valign='middle'>Расчет времени падения</td><td><img src=':/formuls/tpad.png'></td></tr><tr><td valign='middle'>Расчет суммарного времени</td><td><img src=':/formuls/tpol.png'></td></tr><tr><td valign='middle'>Расчяет дальности полета</td><td><img src=':/formuls/maxl.png'></td></tr></table><br><hr><br><b>Для вычисления координаты (x, y) использовались следующие формулы:</b><br><table><tr><td valign='middle'>Координата по Ox</td><td><img src=':/formuls/x.png'></td></tr><tr><td valign='middle'>Координата по Oy</td><td><img src=':/formuls/y.png'></td></tr></table><br><hr><br><b>Начальные параметры:</b><table><tr><td>Начальная высота H<sub>0</sub></td><td>" << graph.h0 << "м</td></tr><tr><td>Начальная скорость V<sub>0</sub></td><td>" << graph.v0 << "м/с</td></tr><tr><td>Угол &alpha;</td><td>" << graph.an << "&deg;</td></tr><tr><td>Ускорение g для планеты " << pl << "    </td><td>" << graph.g << "м/с<sup>2</sup></td></tr></table>" << "<br><hr><br><b>Произведя расчеты по приведенным выше формулам,<br>учитывая, что для планеты " << pl << " g = " << g << "м/с<sup>2</sup>, получим:</b><br><table><tr><td>Максимальная высота полета          </td><td>" << graph.hm << "м</td></tr><tr><td>Время подъема</td><td>" << graph.t1 << "c</td></tr><tr><td>Время падения</td><td>" << graph.t2 << "c</td></tr><tr><td>Время полета</td><td>" << graph.ts << "c</td></tr><tr><td>Максимальная дальность полета</td><td>" << graph.lm << "м</td></tr></table><br><hr><br><b>Используя формулы для расчета координат, построим график движения и тем докажем, что траекторией данного типа движения является парабола.";
	graph.info = info.str().c_str();

	bool exists = false;
	for (int i = 0; i < parmVect.size(); i++) if (graph == parmVect[i]) exists = true;
	if (!exists) parmVect.push_back(graph);
	viewGraphInfo("0");
	gb_animCtl->setEnabled(true);

	if (parmVect.size()) {
		QString buf(""), res("");
		for (int i = 0; i < parmVect.size(); i++) {
			buf.sprintf("<a href='%i' style='color: %s'><b>График №%i</b></a><br>", i, parmVect[i].color.toStdString().c_str(), i + 1);
			res += buf;
		}
		l_graphList->setText(res);
	}

	animPos = 1;
	drawGraph();
}

double Fisika::addAnimPos(double p) {

//	qDebug() << animPos << p;
	double n = animPos;
	n += p;
	if (n < 0 || n > 1.001) return animPos;
	animPos = n;
	return n;
}

double Fisika::setSliderAnimPos(int n) {
	double p = n / 1000.0;
	if (p < 0 || p > 1) return animPos;
	animPos = p;
	animTrue = false;
	QIcon i_animPlay("animButtons/play.jpg"); //изображение для кнопки запуск
	tb_animPause->setIcon(i_animPlay);
	drawGraph();
	return animPos;
}

void Fisika::animation() { //???????????????????????????
	if (animPos <= 1 && animTrue) {
		double mtd = 0;
		for (int i = 0; i < parmVect.size(); i++) {
			mtd = (parmVect[i].ts > mtd) ? parmVect[i].ts : mtd; //максимальное время полета (для анимации)
		}
		double k = spb_timeK->value();
		int wTime = floor(mtd * k);
		animTimer->singleShot(wTime, this, SLOT(animation()));
		stepNext();
	}
}

void Fisika::stepNext() {
	addAnimPos(0.001);
	drawGraph();
	return;
}

void Fisika::stepPrev() {
	addAnimPos(-0.001);
	drawGraph();
	return;
}

void Fisika::stop() {
	animPos = 0;
	animTrue = false;
	animTimer->disconnect(this, SLOT(animation()));
	tb_animPause->setIcon(*i_start);
	drawGraph();
}

void Fisika::pause() {
	animTrue = !animTrue;
	if (animTrue) tb_animPause->setIcon(*i_pause);
	else tb_animPause->setIcon(*i_start);
	animation();
}

void Fisika::aboutQt() {
	QMessageBox::aboutQt(0);
}

void Fisika::viewGraphInfo(QString p) {
	int n = p.toInt();
	if (n < 0 || n > parmVect.size()) return;
	te_graphInfo->setText(parmVect[n].info);
}
