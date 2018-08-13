#ifndef AREA_H
#define AREA_H

#include <vector>

using namespace std;

class Area {
	int N;
	vector<double> x, y, vx, vy, ax, ay, prob, e_sr, P_sr;
	double dt, dv, U, Ek, T, P, kB, T_, sigma, eps, mass;
	unsigned int Steps, Steps0;
	double Lx, Ly;

	double LD(double Rij);
	double Up(double Rij);

public:
	Area(unsigned int, double, double, unsigned int);

	double h();
	double w();

	void initV();
	void initPos();
	void initForce();
	void status();
	void test();

	int nextStep();
	int steps();
	double get_dt();
	bool can_continue();

	vector<double> get_x();
	vector<double> get_y();
	vector<double> get_prob();
	vector<double> sys_parameters();
};


#endif // AREA_H
