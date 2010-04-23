#include "faderwidget.h"

// http://labs.trolltech.com/blogs/2007/08/21/fade-effects-a-blast-from-the-past/

FaderWidget::FaderWidget(QWidget *parent) : QWidget(parent) {
    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(1000, 0);
    connect(timeLine, SIGNAL(frameChanged(int)), this, SLOT(update()));
    setAttribute(Qt::WA_DeleteOnClose);
    resize(parent->size());
}

void FaderWidget::start(QPixmap frozenView) {
    this->frozenView = frozenView;
    timeLine->start();
    show();
}

void FaderWidget::paintEvent(QPaintEvent *) {
    const qreal opacity = timeLine->currentFrame() / 1000.;
    QPainter painter(this);
    painter.setOpacity(opacity);
    painter.drawPixmap(0, 0, frozenView);
    // qDebug() << opacity;

    if (opacity <= 0.)
        close();

}


/*
FaderWidget::FaderWidget(QWidget *oldWidget, QWidget *newWidget) : QWidget(oldWidget) {
    this->oldWidget = oldWidget;
    this->newWidget = newWidget;
    timeLine = new QTimeLine(333, this);
    timeLine->setFrameRange(1000, 0);
    connect(timeLine, SIGNAL(frameChanged(int)), this, SLOT(update()));
    setAttribute(Qt::WA_DeleteOnClose);
    resize(oldWidget->size());
    // hide();
    frozenView = QPixmap::grabWidget(oldWidget);
}

void FaderWidget::start() {
    // oldWidget->setUpdatesEnabled(false);
    // newWidget->setUpdatesEnabled(false);
    qDebug() << "start";
    // setUpdatesEnabled(true);
    show();
    timeLine->start();
}

void FaderWidget::paintEvent(QPaintEvent *) {

    const int currentFrame = timeLine->currentFrame();
    // if (currentFrame == 10) return;

    const qreal opacity = currentFrame / 1000.0;
    qDebug() << "paintEvent" << timeLine->currentFrame() << opacity;
    QPainter painter(this);
    painter.setOpacity(opacity);
    // painter.drawPixmap(0, 0, frozenView);
    painter.setBrush(Qt::red);
    painter.drawRect(rect());

    if (opacity <= 0.) {
        qDebug() << "end fade";
        // oldWidget->setUpdatesEnabled(true);
        // newWidget->setUpdatesEnabled(true);
        close();
    }
}

*/
