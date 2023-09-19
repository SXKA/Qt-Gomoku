#include "windows/mainwindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication qtGobang(argc, argv);
    MainWindow mainWindow;

    mainWindow.setFixedSize(1024, 640);
    mainWindow.setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    mainWindow.show();

    return qtGobang.exec();
}
