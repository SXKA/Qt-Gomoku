#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui_.setupUi(this);
}

void MainWindow::on_ai_released()
{
    bool playerColor;

    if (QMessageBox::question(nullptr, "Stone", "Black? ", QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::NoButton) ==
            QMessageBox::Yes) {
        playerColor = BLACK;
    } else {
        playerColor = WHITE;
    }

    auto *gameWindow = new GameWindow;

    gameWindow->setFixedSize(640, 660);
    gameWindow->setGame(playerColor, AI);
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
    gameWindow->setGame(BLACK, PLAYER);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    this->close();
}
