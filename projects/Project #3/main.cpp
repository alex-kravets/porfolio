#include <QtGui/QApplication>
#include <QDebug>
#include <QTextCodec>

#include "widget.h"
#include "area.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QTextCodec *codec = QTextCodec::codecForName("windows-1251");
	QTextCodec::setCodecForCStrings(codec);

    Widget w;
	w.show();

	return a.exec();
}
