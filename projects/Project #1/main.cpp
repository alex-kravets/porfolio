#include <QtGui/QApplication>
#include "fisika.h"
#include <QTextCodec>

#include <QCleanlooksStyle>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	a.setStyle(new QCleanlooksStyle);
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("CP1251"));

	Fisika main;
	main.show();
	return a.exec();
}
