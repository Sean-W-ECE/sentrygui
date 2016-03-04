#include "sentrygui.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	sentrygui w;
	w.show();
	return a.exec();
}
