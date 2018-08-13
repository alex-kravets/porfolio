#include "area.h"
#include "QTime"
#include <QDebug>
#include <qmath.h>

#define sgn(a) (a < 0 ? -1 : 1)
#define dabs(a) a*sgn(a)

Area::Area(unsigned int n, double lx, double ly, unsigned int steps) : T(0), Steps(0), Steps0(steps)
{
	N = n;
	x = vector<double>(n);
	y = vector<double>(n);
	vx = vector<double>(n);
	vy = vector<double>(n);
	e_sr = vector<double>(n, 0);
	P_sr = vector<double>(n, 0);
	ax = vector<double>(n, 0);
	ay = vector<double>(n, 0);
	prob = vector<double>(40, 0);

	kB = 1.38e-32;
	sigma = 3.405e-10;
	eps = 1.65e-21;
	T_ = eps/kB;
	mass = 6.69e-26;

	dt = 0.000000001;
	dv = 0.2;
	Lx = lx;
	Ly = ly;

	QTime midnight(0, 0, 0);
	qsrand(midnight.secsTo(QTime::currentTime()));

	initV();
	initPos();
	initForce();
}

void Area::initV() {
	for (int i = 0; i < N; i++) {
		vx[i] = 2*(double)qrand()/(double)RAND_MAX - 1;
		vy[i] = 2*(double)qrand()/(double)RAND_MAX - 1;
	}

	double sum_x, sum_y;
	for (int i = 0; i < N; i++) {
		sum_x = sum_y = 0;
		for (int j = 0; j < N; j++) {
			sum_x += vx[j];
			sum_y += vy[j];
		}
		vx[i] -= sum_x/N;
		vy[i] -= sum_y/N;
	}
}

void Area::initPos() {
	QTime midnight(0, 0, 0);
	qsrand(midnight.secsTo(QTime::currentTime()));

	for (int i = 0; i < N; i++) {
		x[i] = Lx*(double)qrand()/(double)RAND_MAX;
		y[i] = Ly*(double)qrand()/(double)RAND_MAX;
	}
}

void Area::status() {
	qDebug() << "N: " << N << ", Lx: " << Lx << ", Ly: " << Ly << "| Steps: " << Steps << ", run for" << Steps0;

	qDebug() << "Position: {";
	for (int i = 0; i < N; i++) {
		qDebug() << "\t(" << x[i] << ", " << y[i] << ")";
	}
	qDebug() << "}" << endl;

	qDebug() << "Velocity: {";
	for (int i = 0; i < N; i++) {
		qDebug() << "\t(" << vx[i] << ", " << vy[i] << ")";
	}
	qDebug() << "}" << endl;

	qDebug() << "Force: {";
	for (int i = 0; i < N; i++) {
		qDebug() << "\t(" << ax[i] << ", " << ay[i] << ")";
	}
	qDebug() << "}" << endl;
}

void Area::initForce() {
	U = 0;

	for (int i = 0; i < N; i++) {
		ax[i] = 0;
		ay[i] = 0;
	}

	double dx, dy, Rij, fij;
	for (int i = 0; i < N-1; i++) {
		for (int j = i+1; j < N; j++) {
			dx = x[i]-x[j];
			if (dabs(dx) > Lx/2) dx -= sgn(dx)*Lx; // ?????
			dy = y[i]-y[j];
			if (dabs(dy) > Ly/2) dy -= sgn(dy)*Ly; // ??????
			Rij = sqrt(dx*dx+dy*dy);
			fij = LD(Rij);
			U += Up(Rij);
			ax[i] = ax[i] + fij*dx;
			ay[i] = ay[i] + fij*dy;
			ax[j] = ax[j] - fij*dx;
			ay[j] = ay[j] - fij*dy;
		}
	}
}

int Area::nextStep() {
	if (Steps >= Steps0) {
		sys_parameters();
		return 0;
	}

	vector<double> vx1(N), vy1(N);
	double _v;
	Ek = 0;

	// первые приближения скоростей
	for (int i = 0; i < N; i++) {
		vx1[i] = vx[i] + 0.5*ax[i]*dt;
		vy1[i] = vy[i] + 0.5*ay[i]*dt;
	}

	// координаты
	for (int i = 0; i < N; i++) {
		x[i] += dt*vx1[i];
		y[i] += dt*vy1[i];

		if (x[i] > Lx) 	x[i] -= Lx;
		if (x[i] < 0) x[i] += Lx;
		if (y[i] > Ly) y[i] -= Ly;
		if (y[i] < 0) y[i] += Ly;
	}

	// силы
	initForce();

	// скорости
	int prob_s = prob.size();
	for (int i = 0; i < N; i++) {
		vx[i] = vx1[i] + 0.5*ax[i]*dt;
		vy[i] = vy1[i] + 0.5*ay[i]*dt;

		_v = vx[i]*vx[i]+vy[i]*vy[i];
		e_sr[i] += _v/2;
		Ek += _v/2;
		for (int j = 0; j < prob_s; j++) {
			if (_v > j*dv && _v < (j+1)*dv) prob[j]+=1;
		}
	}

	Steps++;
	//qDebug() << Steps << (U + Ek);
	//status();
	return 1;
}

//-----------------------------------
double Area::LD(double Rij) {
	return 24*pow(1./Rij, 8.)*(2*pow(1./Rij, 6.)-1.);
}

double Area::Up(double Rij) {
	return 4*pow(1./Rij, 6.)*(pow(1./Rij, 6.)-1.);
}
//-----------------------------------

double Area::h() {
	return Ly;
}

double Area::w() {
	return Lx;
}


vector<double> Area::get_x() {
	return x;
}

vector<double> Area::get_y() {
	return y;
}

vector<double> Area::get_prob() {
	int n = prob.size();
	vector<double> k(n);
	for (int i = 0; i < n; i++) {
		k[i] = prob[i]/Steps;
	}
	return k;
}

int Area::steps() {
	return Steps;
}

vector<double> Area::sys_parameters() {
	//T
	vector<double> res;

	T = 0; T_ = 120;
	for (int i = 0; i < N; i++) {
		T += e_sr[i]/Steps0;
	}
	T *= T_ / N;
	res.push_back(T);

	double tmp = 0;
	for (int i = 0; i < N; i++) {
		tmp += P_sr[i];
	}
	tmp /= 2*Steps0;
	P = kB*T*N + eps*tmp;
	P /= Lx*Ly*sigma*sigma;
	res.push_back(P);

	//qDebug() << T << P;
	return res;
}

bool Area::can_continue() {
	return Steps < Steps0;
}

double Area::get_dt() {
	return dt;
}

void Area::test() {
	double k = 1;
	for (int i = 0; i < 64; i+=8) {
		for (int j = 0; j < 8; j++) {
			int t = i*8+j;
			x[i+j] = (j+1)*0.1;
			y[i+j] = (i/8+1)*0.1;
		}
	}
	initV();
	initForce();
}
