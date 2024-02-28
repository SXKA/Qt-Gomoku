#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"
#include "GameWindow.h"

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

protected slots:
    void on_exit_released();
    void on_pvc_released();
    void on_pvp_released();
private:
    Ui::MainWindowClass ui_;
};

#endif
