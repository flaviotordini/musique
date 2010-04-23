#include "tracklistview.h"
#include "trackitemdelegate.h"
#include "database.h"
#include <QtSql>
#include "tracksqlmodel.h"

TrackListView::TrackListView(QWidget *parent) : QListView(parent) {
    setWindowTitle(tr("Tracks"));

    this->setItemDelegate(new TrackItemDelegate(this));
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->setMouseTracking(true);

    // layout
    this->setResizeMode(QListView::Adjust);
    this->setMovement(QListView::Static);
    this->setUniformItemSizes(true);

    // colors
    QPalette p = palette();
    p.setBrush(QPalette::Base, Qt::black);
    p.setBrush(QPalette::Text, Qt::white);
    this->setPalette(p);

    // dragndrop
    this->setDragEnabled(true);
    // this->setAcceptDrops(true);
    // this->setDropIndicatorShown(true);
    this->setDragDropMode(QAbstractItemView::DragOnly);

    // cosmetics
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setFrameShape( QFrame::NoFrame );
    this->setAttribute(Qt::WA_MacShowFocusRect, false);

    verticalScrollBar()->setPageStep(3);
    verticalScrollBar()->setSingleStep(1);

}

void TrackListView::appear() {

}
