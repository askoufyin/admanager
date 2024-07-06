#include "AdManager.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AdManager w;
    w.show();
    return a.exec();
}
