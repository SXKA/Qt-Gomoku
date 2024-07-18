#include "windows/mainwindow.h"

#include <QStyleFactory>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication qtGomoku(argc, argv);
    MainWindow mainWindow;

    qtGomoku.setStyle(QStyleFactory::create("Fusion"));

    mainWindow.setFixedSize(1024, 640);
    mainWindow.setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    mainWindow.show();

    return qtGomoku.exec();
}
