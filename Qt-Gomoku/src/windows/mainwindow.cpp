#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui_.setupUi(this);
}

void MainWindow::on_ai_released()
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
    gameWindow->setGame(playerStone, AI);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    this->close();
}


void MainWindow::on_exit_released()
{
    this->close();
}

void MainWindow::on_player_released()
{
    auto *gameWindow = new GameWindow;

    gameWindow->setFixedSize(640, 660);
    gameWindow->setGame(Gomoku::Black, PLAYER);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    this->close();
}
