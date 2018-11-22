#include "minisplitter.h"

#include <QtWidgets>

class MiniSplitterHandle : public QSplitterHandle {
public:
    MiniSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent) {
        setMask(QRegion(contentsRect()));
        setAttribute(Qt::WA_MouseNoMask);
        setCursor(Qt::SizeHorCursor);
    }

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
};

void MiniSplitterHandle::resizeEvent(QResizeEvent *event) {
    if (orientation() == Qt::Horizontal)
        setContentsMargins(2, 0, 2, 0);
    else
        setContentsMargins(0, 2, 0, 2);
    setMask(QRegion(contentsRect()));
    QSplitterHandle::resizeEvent(event);
}

void MiniSplitterHandle::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
}

QSplitterHandle *MiniSplitter::createHandle() {
    return new MiniSplitterHandle(orientation(), this);
}

MiniSplitter::MiniSplitter(QWidget *parent) : QSplitter(parent) {
    setHandleWidth(0);
    setChildrenCollapsible(false);
}

MiniSplitter::MiniSplitter(Qt::Orientation orientation) : QSplitter(orientation) {
    setHandleWidth(1);
    setChildrenCollapsible(false);
}
