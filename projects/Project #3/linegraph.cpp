#include "linegraph.h"

#include <QPainter>

LineGraph::LineGraph(QWidget * parent) : QWidget(parent)
{
}


void LineGraph::paintEvent(QPaintEvent *) {
	QPainter p(this);
	QColor grid_lines(190, 190, 190);

	p.setPen(QPen(grid_lines, 1, Qt::SolidLine));

	double area_h = height();
	double area_w = width();
	int grid_step = 10;

	for (int i = grid_step; i < area_w; i+=grid_step) {
		p.drawLine(i, 0, i, area_h);
	}

	for (int i = grid_step; i < area_h; i+=grid_step) {
		p.drawLine(0, i, area_w, i);
	}

	p.setPen(QPen(Qt::black, 1, Qt::SolidLine));
	// y - axis
	p.drawLine(20, 20, 20, area_h - 20);
	p.drawLine(20, 20, 18, 40);
	p.drawLine(20, 20, 22, 40);
	//x - axis
	p.drawLine(20, area_h - 20, area_w - 20, area_h - 20);
	p.drawLine(area_w - 20, area_h - 20, area_w - 40, area_h - 22);
	p.drawLine(area_w - 20, area_h - 20, area_w - 40, area_h - 18);
	// to (0,0)
	p.translate(20, area_h - 20);

	//scaling
	int n_steps = prob.size();
	if (n_steps) {
		double v_max = prob[0];
		if (v_max == 0) return;
		for (int i = 0; i < n_steps; i++) if (prob[i] > v_max) v_max = prob[i];
		double kx = (area_w - 20-20-30) / n_steps;
		double ky = (area_h - 20-20-30) / v_max;

		//draw
		for (int i = 0; i < n_steps; i++) {
			p.fillRect(i*kx, 0, kx, -prob[i]*ky, QBrush(Qt::red, Qt::Dense4Pattern));
		}
	}
}

void LineGraph::setProb(vector<double> prb) {
	prob = prb;
	this->repaint();
}
