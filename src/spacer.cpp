#include "spacer.h"

Spacer::Spacer(QWidget *parent) : QWidget(parent), width(20) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setEnabled(false);
}

Spacer::Spacer(int width, QWidget *parent) : QWidget(parent), width(width) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setEnabled(false);
}

QSize Spacer::sizeHint() const {
    return QSize(width, 1);
}
