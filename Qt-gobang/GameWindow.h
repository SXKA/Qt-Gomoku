#pragma once

#include "gobang.h"
#include "ui_GameWindow.h"
#include "MainWindow.h"
#include <QtEvents>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QPainter>
#include <QPoint>


#define AI false
#define PLAYER true

#define BLACK false
#define WHITE true

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    GameWindow(QWidget *parent = nullptr);
    void setGame(const bool &color, const bool &type);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

protected slots:
    void on_async_finished();
    void on_back_triggered();
    void on_exit_triggered();
    void on_newGame_triggered();
    void on_menu_aboutToShow() const;

private:
    Ui::GameWindowClass ui;
    QFutureWatcher<void> watcher;
    QFuture<void> future;
    QPoint last;
    QPoint move;
    gobang::Gobang gobang;
    bool gameType;
    bool gameOver;
    bool playerColor;
};