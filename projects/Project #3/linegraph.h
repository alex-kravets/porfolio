#ifndef LINEGRAPH_H
#define LINEGRAPH_H

#include <QWidget>
#include <vector>

using namespace std;

class LineGraph : public QWidget
{
	Q_OBJECT

	vector<double> prob;

public:
	LineGraph(QWidget * parent = 0);
	void setProb(vector<double> prb);

protected:
	void paintEvent(QPaintEvent *);
};

#endif // LINEGRAPH_H
