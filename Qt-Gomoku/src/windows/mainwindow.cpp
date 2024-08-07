#include "mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui_.setupUi(this);
}

void MainWindow::on_exit_released()
{
    close();
}

void MainWindow::on_pvc_released()
{
    Stone playerStone;

    if (QMessageBox::question(nullptr,
                              "Stone",
                              "Black? ",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::NoButton)
        == QMessageBox::Yes) {
        playerStone = Black;
    } else {
        playerStone = White;
    }

    auto *gameWindow = new GameWindow;

    gameWindow->setFixedSize(640, 660);
    gameWindow->setGame(playerStone, PVC);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    close();
}

void MainWindow::on_pvp_released()
{
    const auto gameWindow = new GameWindow;

    gameWindow->setFixedSize(640, 660);
    gameWindow->setGame(Black, PVP);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    close();
}
