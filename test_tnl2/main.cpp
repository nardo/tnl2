#include <QtGui>
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	window the_window;
	the_window.resize(QSize(800, 400));
	the_window.show();
    return a.exec();
}
