#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include "../gobang/engine.h"
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

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    GameWindow(QWidget *parent = nullptr);
    void setGame(const Gobang::Stone &stone, const bool &type);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

protected slots:
    void on_async_finished();
    void on_undo_triggered();
    void on_exit_triggered() const;
    void on_newGame_triggered();
    void on_menu_aboutToShow() const;

private:
    Ui::GameWindowClass ui;
    QFutureWatcher<void> watcher;
    QFuture<void> future;
    QPoint last;
    QPoint move;
    Gobang::Engine engine;
    Gobang::Stone playerStone;
    bool gameOver;
    bool gameType;
};

#endif
