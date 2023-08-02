#pragma once

#include "ui_MainWindow.h"
#include "GameWindow.h"
#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

protected slots:
    void on_ai_released();
    void on_exit_released();
    void on_player_released();

private:
    Ui::MainWindowClass ui_;
};