#include "AdManager.h"
#include <QtWidgets/QApplication>
#include "GlobalAppConfig.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AdManager w;

    switch (globalConfig.windowOnStart) {
    case Start_Window_Maximized:
        w.showMaximized();
        break;
    case Start_Window_Minimized:
        w.showMinimized();
        break;
    case Start_Window_Hidden:
        w.hide();
        break;
    default:
        w.show();
        break;
    }

    return a.exec();
}
