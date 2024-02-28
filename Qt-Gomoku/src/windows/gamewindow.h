#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include "../search/engine.h"
#include "ui_GameWindow.h"

#include <QFuture>
#include <QFutureWatcher>
#include <QMainWindow>
#include <QPoint>

constexpr auto PVC = false;
constexpr auto PVP = true;

class GameWindow : public QMainWindow
{
    Q_OBJECT
private:
    Ui::GameWindowClass ui;
    QFutureWatcher<void> watcher;
    QFuture<void> future;
    QPoint last;
    QPoint move;
    Search::Engine engine;
    Stone playerStone;
    int step;
    bool gameOver;
    bool gameType;
public:
    GameWindow(QWidget *parent = nullptr);
    void setGame(const Stone &stone, const bool &type);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

protected slots:
    void on_async_finished();
    void on_exit_released();
    void on_newGame_released();
    void on_undo_released();
};

#endif
