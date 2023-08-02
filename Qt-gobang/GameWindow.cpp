#include "GameWindow.h"

GameWindow::GameWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    future = QFuture<void>();
    last = QPoint();
    move = QPoint();
    gobang = gobang::Gobang();
    gameType = AI;
    gameOver = false;
    playerColor = BLACK;

    connect(&watcher, &QFutureWatcher<void>::finished, this, &GameWindow::on_async_finished);
}

void GameWindow::mouseMoveEvent(QMouseEvent *event)
{
    move.setX((event->pos().y() - 40) / 40);
    move.setY((event->pos().x() - 20) / 40);

    update();
}

void GameWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (future.isValid() && future.isRunning()) {
        return;
    }

    if (gameOver) {
        return;
    }

    if (gobang::Gobang::isLegal(move) && gobang.checkStone(move) == gobang::empty) {
        gobang.play(move, static_cast<const gobang::stone>(playerColor));
    } else {
        return;
    }

    last = gobang.lastStone();

    repaint();

    if (gobang.gameOver(last, static_cast<const gobang::stone>(playerColor))) {
        gameOver = true;

        return;
    }

    if (gameType == AI) {
        ui.menu->setDisabled(true);
        ui.menuBar->setDisabled(true);

        setUpdatesEnabled(false);

        future = QtConcurrent::run([ &, this]() {
            const auto aiStone = static_cast<const gobang::stone>(!playerColor);

            gobang.play(gobang.ai(aiStone), aiStone);

            last = gobang.lastStone();
        });

        watcher.setFuture(future);
    } else {
        playerColor = !playerColor;
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
            if (gobang.checkStone(QPoint(i, j)) == static_cast<const gobang::stone>(BLACK)) {
                brush.setColor(Qt::black);

                painter.setBrush(brush);
                painter.drawEllipse(QPoint((j + 1) * 40, (i + 1) * 40 + 20), 18, 18);
            } else if (gobang.checkStone(QPoint(i, j)) == static_cast<const gobang::stone>(WHITE)) {
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

    if (!last.isNull()) {
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

void GameWindow::setGame(const bool &color, const bool &type)
{
    playerColor = color;
    gameType = type;

    if (playerColor == WHITE && gameType == AI) {
        gobang.play(QPoint(7, 7), static_cast<const gobang::stone>(BLACK));

        last = gobang.lastStone();
    }
}

void GameWindow::on_async_finished()
{
    ui.menu->setEnabled(true);
    ui.menuBar->setEnabled(true);

    setUpdatesEnabled(true);
    repaint();

    if (gobang.gameOver(last, static_cast<const gobang::stone>(!playerColor))) {
        gameOver = true;
    }
}


void GameWindow::on_back_triggered()
{
    gameOver = false;

    QPoint point;

    if (gameType == AI) {
        gobang.back(2);
    } else {
        gobang.back(1);

        playerColor = !playerColor;
    }

    last = gobang.lastStone();

    update();
}


void GameWindow::on_exit_triggered()
{
    this->deleteLater();
}

void GameWindow::on_newGame_triggered()
{
    auto *gameWindow = new GameWindow;

    if (gameType == AI) {
        if (QMessageBox::question(nullptr, "Color", "Black? ", QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::NoButton) == QMessageBox::Yes) {
            playerColor = BLACK;
        } else {
            playerColor = WHITE;
        }
    }

    gameWindow->setGame(playerColor, gameType);
    gameWindow->setFixedSize(640, 660);
    gameWindow->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    gameWindow->show();

    this->deleteLater();
}

void GameWindow::on_menu_aboutToShow() const
{
    if (gobang.isInitial(gameType, static_cast<const gobang::stone>(playerColor))) {
        ui.back->setVisible(false);
    } else {
        ui.back->setVisible(true);
    }
}