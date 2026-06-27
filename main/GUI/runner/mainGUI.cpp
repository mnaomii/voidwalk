#include "../widgets/mainWindow.h"
#include <QtWidgets/QApplication>

int GUIstart(int argc, char *argv[])
{
    QApplication app(argc, argv);
    mainWindow window;
    window.show();
    return app.exec();
}
