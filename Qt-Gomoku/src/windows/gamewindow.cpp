#include "gamewindow.h"

GameWindow::GameWindow(QWidget *parent)
    : QMainWindow(parent)
    , last(QPoint(-1, -1))
    , playerStone(Gomoku::Black)
    , step(0)
    , gameOver(false)
    , gameType(PVC)
{
    ui.setupUi(this);

    connect(qApp, &QApplication::aboutToQuit, &watcher, &QFutureWatcher<void>::waitForFinished);
    connect(&watcher, &QFutureWatcher<void>::finished, this, &GameWindow::on_async_finished);
}

void GameWindow::mouseMoveEvent(QMouseEvent *event)
{
    const auto x = event->pos().x();
    const auto y = event->pos().y();

    move.setX((y - 40) / 40);
    move.setY((x - 20) / 40);

    if (x < 20 || x >= 620 || y < 40 || y >= 640) {
        setCursor(Qt::ArrowCursor);
    } else {
        if (engine.checkStone(move) == Gomoku::Empty) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }

    update();
}

void GameWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        return;
    }

    const auto x = event->pos().x();
    const auto y = event->pos().y();

    if (x < 20 || x >= 620 || y < 30 || y >= 640) {
        return;
    }

    if (future.isValid() && future.isRunning()) {
        return;
    }

    if (gameOver) {
        return;
    }

    if (engine.checkStone(move) == Gomoku::Empty) {
        engine.move(move, playerStone);
    } else {
        return;
    }

    ui.undo->setEnabled(true);
    last = engine.lastMove();
    ++step;

    repaint();

    const auto gameState = engine.gameState(move, playerStone);

    if (gameState == Gomoku::Draw || gameState == Gomoku::Win) {
        gameOver = true;

        if (gameState == Gomoku::Draw) {
            QMessageBox::information(nullptr, "Result", "Draw!", QMessageBox::Ok, QMessageBox::NoButton);
        } else {
            QString winner = playerStone == Gomoku::Black ? "Black" : "White";

            winner.append(" win!");

            QMessageBox::information(nullptr, "Result", winner, QMessageBox::Ok, QMessageBox::NoButton);
        }

        if (gameType == PVP) {
            playerStone = static_cast<const Gomoku::Stone>(-playerStone);
        }

        return;
    }

    if (gameType == PVC) {
        ui.undo->setDisabled(true);

        repaint();
        setUpdatesEnabled(false);

        future = QtConcurrent::run([ &, this]() {
            const auto stone = static_cast<const Gomoku::Stone>(-playerStone);

            engine.move(engine.bestMove(stone), stone);

            last = engine.lastMove();
        });

        watcher.setFuture(future);
    } else {
        playerStone = static_cast<const Gomoku::Stone>(-playerStone);
    }
}


void GameWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);

    QPalette palette;

    palette.setColor(QPalette::Window, QColor("#b1723c"));

    this->setPalette(palette);

    QPen pen = painter.pen();

    pen.setColor(QColor("#8d5822"));
    pen.setWidth(7);

    painter.setPen(pen);

    QBrush brush;

    brush.setColor(QColor("#eec085"));
    brush.setStyle(Qt::SolidPattern);

    painter.setBrush(brush);
    painter.drawRect(20, 40, 600, 600);

    pen.setColor(Qt::black);
    pen.setWidth(1);

    painter.setPen(pen);

    for (int i = 0; i < 15; ++i) {
        painter.drawLine(40 + i * 40, 60, 40 + i * 40, 620);
        painter.drawLine(40, 60 + i * 40, 600, 60 + i * 40);
    }

    brush.setColor(Qt::black);

    painter.setBrush(brush);
    painter.drawEllipse(155, 175, 10, 10);
    painter.drawEllipse(475, 175, 10, 10);
    painter.drawEllipse(155, 495, 10, 10);
    painter.drawEllipse(475, 495, 10, 10);
    painter.drawEllipse(315, 335, 10, 10);

    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            if (engine.checkStone(QPoint(i, j)) == Gomoku::Black) {
                brush.setColor(Qt::black);

                painter.setBrush(brush);
                painter.drawEllipse(QPoint((j + 1) * 40, (i + 1) * 40 + 20), 18, 18);
            } else if (engine.checkStone(QPoint(i, j)) == Gomoku::White) {
                brush.setColor(Qt::white);

                painter.setPen(Qt::NoPen);
                painter.setBrush(brush);
                painter.drawEllipse(QPoint((j + 1) * 40, (i + 1) * 40 + 20), 18, 18);
            }
        }
    }

    pen.setColor(Qt::red);
    pen.setWidth(1);

    painter.setPen(pen);

    if ((move.x() * 40 + 40) >= 40 && (move.x() * 40 + 40) <= 620 && (move.y() * 40 + 20) >= 20
            && (move.y() * 40 +
                20) <= 600) {
        painter.drawLine((move.y() + 1) * 40 - 20, (move.x() + 1) * 40, (move.y() + 1) * 40 - 10,
                         (move.x() + 1) * 40);
        painter.drawLine((move.y() + 1) * 40 + 20, (move.x() + 1) * 40, (move.y() + 1) * 40 + 10,
                         (move.x() + 1) * 40);
        painter.drawLine((move.y() + 1) * 40 - 20, (move.x() + 1) * 40 + 40, (move.y() + 1) * 40 - 10,
                         (move.x() + 1) * 40 + 40);
        painter.drawLine((move.y() + 1) * 40 + 20, (move.x() + 1) * 40 + 40, (move.y() + 1) * 40 + 10,
                         (move.x() + 1) * 40 + 40);
        painter.drawLine((move.y() + 1) * 40 - 20, (move.x() + 1) * 40, (move.y() + 1) * 40 - 20,
                         (move.x() + 1) * 40 + 10);
        painter.drawLine((move.y() + 1) * 40 + 20, (move.x() + 1) * 40, (move.y() + 1) * 40 + 20,
                         (move.x() + 1) * 40 + 10);
        painter.drawLine((move.y() + 1) * 40 - 20, (move.x() + 1) * 40 + 40, (move.y() + 1) * 40 - 20,
                         (move.x() + 1) * 40 + 30);
        painter.drawLine((move.y() + 1) * 40 + 20, (move.x() + 1) * 40 + 40, (move.y() + 1) * 40 + 20,
                         (move.x() + 1) * 40 + 30);
    }

    if (last != QPoint(-1, -1)) {
        painter.drawLine((last.y() + 1) * 40 - 1, (last.x() + 1) * 40 + 20, (last.y() + 1) * 40 - 6,
                         (last.x() + 1) * 40 + 20);
        painter.drawLine((last.y() + 1) * 40 + 1, (last.x() + 1) * 40 + 20, (last.y() + 1) * 40 + 6,
                         (last.x() + 1) * 40 + 20);
        painter.drawLine((last.y() + 1) * 40, (last.x() + 1) * 40 + 19, (last.y() + 1) * 40,
                         (last.x() + 1) * 40 + 14);
        painter.drawLine((last.y() + 1) * 40, (last.x() + 1) * 40 + 21, (last.y() + 1) * 40,
                         (last.x() + 1) * 40 + 26);
    }
}

void GameWindow::setGame(const Gomoku::Stone &stone, const bool &type)
{
    playerStone = stone;
    gameType = type;

    if (playerStone == Gomoku::White && gameType == PVC) {
        engine.move(engine.bestMove(static_cast<const Gomoku::Stone>(-playerStone)), Gomoku::Black);

        last = engine.lastMove();
    }
}

void GameWindow::on_async_finished()
{
    ui.undo->setEnabled(true);

    setUpdatesEnabled(true);
    repaint();

    const auto stone = static_cast<const Gomoku::Stone>(-playerStone);
    const auto gameState = engine.gameState(engine.lastMove(), stone);

    if (gameState == Gomoku::Draw || gameState == Gomoku::Win) {
        gameOver = true;

        if (gameState == Gomoku::Draw) {
            QMessageBox::information(nullptr, "Result", "Draw!", QMessageBox::Ok, QMessageBox::NoButton);
        } else {
            QString winner = stone == Gomoku::Black ? "Black" : "White";

            winner.append(" win!");

            QMessageBox::information(nullptr, "Result", winner, QMessageBox::Ok, QMessageBox::NoButton);
        }
    }
}

void GameWindow::on_exit_released()
{
    this->close();
}

void GameWindow::on_newGame_released()
{
    const auto gameWindow = new GameWindow;

    playerStone = Gomoku::Black;

    if (gameType == PVC) {
        if (QMessageBox::question(nullptr, "Stone", "Black? ", QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::NoButton) == QMessageBox::No) {
            playerStone = Gomoku::White;
        }
    }

    gameWindow->setGame(playerStone, gameType);
    gameWindow->setFixedSize(640, 660);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    watcher.waitForFinished();

    this->deleteLater();
}

void GameWindow::on_undo_released()
{
    if (!(--step)) {
        ui.undo->setDisabled(true);
    }

    gameOver = false;

    if (gameType == PVC) {
        engine.undo(2);
    } else {
        engine.undo(1);

        playerStone = static_cast<const Gomoku::Stone>(-playerStone);
    }

    last = engine.lastMove();

    update();
}
