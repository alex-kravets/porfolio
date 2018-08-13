#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "area.h"
#include "linegraph.h"
#include "atomarea.h"

class Widget : public QWidget
{
    Q_OBJECT

	LineGraph* Maxwell;
	AtomArea* AtomScene;


	QLineEdit* te_n;
	QLineEdit* te_width;
	QLineEdit* te_height;
	QLineEdit* te_steps;

	QLabel* l_sysinfo;

	QPushButton* pb_start;
	QPushButton* pb_pause;
	QPushButton* pb_reset;

public:
    Widget(QWidget *parent = 0);
    ~Widget();

public slots:
	void initData();
	void pauseSim();
	void draw_info();
	void error(QString);
	void test();

};

#endif // WIDGET_H
