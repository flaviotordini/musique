#include "framelesswindow.h"

namespace {
const char *framelessKey = "frameless";
}

FramelessWindow::FramelessWindow(QWidget *parent)
    : QMainWindow(parent), enabled(false), mainToolBar(nullptr) {
    originalWindowFlags = windowFlags();
}

void FramelessWindow::enableFrameless() {
    enabled = true;

    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);

    /*
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::CustomizeWindowHint;
    flags &= ~Qt::WindowMinimizeButtonHint;
    flags &= ~Qt::WindowMaximizeButtonHint;
    flags &= ~Qt::WindowCloseButtonHint;
    flags &= ~Qt::WindowTitleHint;
    setWindowFlags(flags);
    */

    setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect;
    const int blurRadius = 9 * 4;
    effect->setBlurRadius(blurRadius);
    effect->setColor(QColor(0, 0, 0, 64));
    effect->setYOffset(3);
    setGraphicsEffect(effect);
    setContentsMargins(blurRadius, blurRadius - effect->yOffset(), blurRadius,
                       blurRadius + effect->yOffset());

    setProperty(framelessKey, true);
    style()->unpolish(this);
    style()->polish(this);

    if (mainToolBar) {
        style()->unpolish(mainToolBar);
        style()->polish(mainToolBar);
        /*
        mainToolBar->setStyleSheet("QToolbar{border: 1px solid red;"
                                   "border-radius: 20px;"
                                   "background-color: black;}");
    */
        // mainToolBar->setStyleSheet("border:0;padding:5px 10px");
    }

    // TODO QSettings
}

void FramelessWindow::disableFrameless() {
    enabled = false;
    setWindowFlags(originalWindowFlags);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setGraphicsEffect(nullptr);
    setProperty(framelessKey, QVariant());
    style()->unpolish(this);
    style()->polish(this);
}

void FramelessWindow::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    if (!enabled) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    // isActiveWindow() ? p.setBrush(QColor(0x2b, 0x29, 0x29)) : p.setBrush(QColor(0x3d, 0x3a,
    // 0x3a));
    p.setBrush(QColor(0x2b, 0x29, 0x29));

    p.drawRoundedRect(contentsRect(), 7, 7);
}
