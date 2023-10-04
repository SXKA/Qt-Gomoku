#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui_.setupUi(this);
}

void MainWindow::on_exit_released()
{
    this->close();
}

void MainWindow::on_pvc_released()
{
    Gomoku::Stone playerStone;

    if (QMessageBox::question(nullptr, "Stone", "Black? ", QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::NoButton) ==
            QMessageBox::Yes) {
        playerStone = Gomoku::Black;
    } else {
        playerStone = Gomoku::White;
    }

    auto *gameWindow = new GameWindow;

    gameWindow->setFixedSize(640, 660);
    gameWindow->setGame(playerStone, PVC);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    this->close();
}

void MainWindow::on_pvp_released()
{
    const auto gameWindow = new GameWindow;

    gameWindow->setFixedSize(640, 660);
    gameWindow->setGame(Gomoku::Black, PVP);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    this->close();
}
