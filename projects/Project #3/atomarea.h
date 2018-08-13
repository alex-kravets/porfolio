#ifndef ATOMAREA_H
#define ATOMAREA_H

#include <QWidget>
#include <QThread>
#include <vector>

#include "area.h"

using namespace std;

class AtomArea : public QWidget, public QThread
{
	Q_OBJECT

	double k;
	double aw, ah;

public:
	bool torun;
	Area * A;

	AtomArea(QWidget * parent = 0);
	void init(int n, double dx, double dy, int steps);

	void run();
	void ps();

protected:
	void paintEvent(QPaintEvent *);
};

#endif // ATOMAREA_H
