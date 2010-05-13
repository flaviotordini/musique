#include <QtGui>
#include "droparea.h"
#include "iconloader/qticonloader.h"

DropArea::DropArea(QWidget *parent) : QLabel(parent) {
    setMargin(15);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}

void DropArea::dragEnterEvent(QDragEnterEvent *event) {
    setBackgroundRole(QPalette::Highlight);
    setForegroundRole(QPalette::HighlightedText);

    event->acceptProposedAction();
    emit changed(event->mimeData());
}

void DropArea::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

void DropArea::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();

    // TODO

    event->acceptProposedAction();
}

void DropArea::dragLeaveEvent(QDragLeaveEvent *event) {
    clear();
    event->accept();
}

void DropArea::clear() {
    setText("<b>" + tr("Drop here to append to the playlist") + "</b>");
    setPixmap(QtIconLoader::icon("list-add", QIcon(":/images/list-add.png")).pixmap(24, 24));
    setBackgroundRole(QPalette::Base);
    setForegroundRole(QPalette::Text);

    emit changed();
}
