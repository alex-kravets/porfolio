#include "atomarea.h"
#include "area.h"

#include <QPainter>
#include <QImage>
#include <QDebug>

AtomArea::AtomArea(QWidget* parent) : QWidget(parent)
{
	A = 0;
	torun = true;
}

void AtomArea::paintEvent(QPaintEvent *) {
	QPainter p(this);

	QPen grid_lines(QColor(190, 190, 190), 1, Qt::SolidLine);
	QPen border(Qt::blue, 3, Qt::SolidLine);
	QPen atoms(Qt::red, 3, Qt::SolidLine);

	double k1 = width() / aw;
	double k2 = height() / ah;
	k = (k1 < k2 ? k1 : k2);

	int x_margin = (width() - aw * k) / 2;
	int y_margin = (height() - ah * k) / 2;
	p.translate(x_margin, y_margin);

// grid
	p.setPen(grid_lines);

	int area_h = ah*k;
	int area_w = aw*k;
	int grid_step = 20;

	for (int i = grid_step; i < area_w; i+=grid_step) {
		p.drawLine(i, 0, i, area_h);
	}

	for (int i = grid_step; i < area_h; i+=grid_step) {
		p.drawLine(0, i, area_w, i);
	}

// border
	p.setPen(border);

	int dx = aw*k, dy = ah*k;

	p.drawLine(0, 0, dx, 0);
	p.drawLine(0, dy, dx, dy);
	p.drawLine(0, 0, 0, dy);
	p.drawLine(dx, 0, dx, dy);

	if(A) {
		vector<double> x = A->get_x();
		vector<double> y = A->get_y();
		p.setPen(atoms);
		int points = x.size();
		for (int i = 0; i < points; i++) {
			p.drawPoint(x[i]*k, y[i]*k);
		}
	}
}

void AtomArea::init(int n, double dx, double dy, int steps) {
	A = new Area(n, dx, dy, steps);
	aw = dx;
	ah = dy;
	update();
	torun = true;
	start();
}

void AtomArea::run() {
	while (torun && A->can_continue()) {
		A->nextStep();
		update();
		AtomArea::msleep(70);
	}
}

void AtomArea::ps() {
	torun = !torun;
	this->start();
}
