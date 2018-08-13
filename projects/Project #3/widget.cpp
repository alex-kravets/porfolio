#include "widget.h"
#include "linegraph.h"
#include "atomarea.h"
#include "area.h"
#include <QtGui>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{		
	this->setWindowTitle("Project 1");
	this->setFixedSize(1000, 500);

	QVBoxLayout* vl_main = new QVBoxLayout();
	QHBoxLayout* hl_top = new QHBoxLayout();
	QHBoxLayout* hl_bottom = new QHBoxLayout();


// ����� ����
	QGroupBox* left_menu = new QGroupBox("��������� �������");
	QVBoxLayout* vl_menu = new QVBoxLayout();


	//QLabel* l_params = new QLabel("<b><u>��������� �������</u></b>");
	QLabel* l_atoms = new QLabel("���-�� ������:");
	QLabel* l_width = new QLabel("������ ����:");
	QLabel* l_height = new QLabel("����� ����:");
	QLabel* l_steps = new QLabel("���������� �����:");

	te_n = new QLineEdit();
	te_width = new QLineEdit();
	te_height = new QLineEdit();
	te_steps = new QLineEdit();

	pb_start = new QPushButton("������");
	connect(pb_start, SIGNAL(clicked()), this, SLOT(initData()));
	pb_pause = new QPushButton("�������������");
	connect(pb_pause, SIGNAL(clicked()), this, SLOT(pauseSim()));
	QPushButton* pb_solve = new QPushButton("����������");
	//pb_reset = new QPushButton("�����");

	connect(pb_solve, SIGNAL(clicked()), this, SLOT(draw_info()));

	//vl_menu->addWidget(l_params);
    vl_menu->addWidget(l_atoms);
	vl_menu->addWidget(te_n);
	vl_menu->addWidget(l_width);
	vl_menu->addWidget(te_width);
	vl_menu->addWidget(l_height);
	vl_menu->addWidget(te_height);
	vl_menu->addWidget(l_steps);
	vl_menu->addWidget(te_steps);
	vl_menu->addWidget(pb_start);
	vl_menu->addWidget(pb_pause);
	vl_menu->addWidget(pb_solve);
	//vl_menu->addWidget(pb_reset);

	vl_menu->addStretch(1);

	left_menu->setFixedWidth(140);
	left_menu->setLayout(vl_menu);

// ���� � ���������
	QGroupBox* model_area = new QGroupBox("������� ������");
	QVBoxLayout* model_main = new QVBoxLayout();


	AtomScene = new AtomArea();
	AtomScene->setFixedWidth(400);

	model_main->addWidget(AtomScene);
	model_area->setLayout(model_main);

// ������������� ���������
	QGroupBox* maxwell_graph = new QGroupBox("������������� �� ���������");
	QVBoxLayout* maxwell_main = new QVBoxLayout();

	l_sysinfo = new QLabel("���������� � �������");

	Maxwell = new LineGraph();
	Maxwell->setFixedWidth(400);
	Maxwell->setFixedHeight(200);

	maxwell_main->addWidget(Maxwell);
	maxwell_main->addWidget(l_sysinfo);
	maxwell_graph->setLayout(maxwell_main);

// ������� ����
	hl_top->addWidget(left_menu);
	hl_top->addWidget(model_area);
	hl_top->addWidget(maxwell_graph);

// ������ ����
	QWidget* bottom = new QWidget();
	QHBoxLayout* l_bt = new QHBoxLayout();
	QPushButton* pb_exit = new QPushButton("�����");
	connect(pb_exit, SIGNAL(clicked()), this, SLOT(close()));

	l_bt->addStretch(1);
	l_bt->addWidget(pb_exit);
	bottom->setLayout(l_bt);
	bottom->setFixedHeight(60);

// ������� ����
	vl_main->addLayout(hl_top);
	//vl_main->addLayout(hl_bottom);
	vl_main->addWidget(bottom);
	this->setLayout(vl_main);

	test();
}

void Widget::test() {
	AtomScene->init(64, 0.9, 0.9, 300);
	AtomScene->A->test();
}

Widget::~Widget()
{
}


void Widget::initData() {
	int n = te_n->text().toInt();
	if (n <= 0)	{
		error("���������� ������ ������� �������!");
		return;
	}

	double dx = te_width->text().toDouble();
	if (dx <= 0)	{
		error("������ ���� ������� �������!");
		return;
	}

	double dy = te_height->text().toDouble();
	if (dy <= 0)	{
		error("����� ���� ������� �������!");
		return;
	}

	int steps = te_steps->text().toInt();
	if (steps <= 0) {
		error("���������� ����� ������� �������!");
		return;
	}

	AtomScene->init(n, dx, dy, steps);

}

void Widget::draw_info() {
	if (!AtomScene->A) return;
	Maxwell->setProb(AtomScene->A->get_prob());
	vector<double> pars = AtomScene->A->sys_parameters();
	QString tt = QString::number(pars[0]);
	QString pp = QString::number(pars[1]);
	QString ss = QString::number(AtomScene->A->steps());
	QString dt = QString::number(AtomScene->A->get_dt());
	QString lf = QString::number(AtomScene->A->get_dt() * AtomScene->A->steps());
	l_sysinfo->setText("<center><b>���������� ��������� �������:</b></center><b>T:</b> " + tt + " �<br /><b>P: </b> " + pp + " ��<br /><br />�����: " + ss + ", ���������� �� �������: " + dt + "<br>����� ����� �������: " + lf);
}

void Widget::pauseSim() {
	if (!AtomScene->A) return;
	draw_info();
	AtomScene->ps();
}


void Widget::error(QString s) {
	QMessageBox::warning(this, "������", s);
}
