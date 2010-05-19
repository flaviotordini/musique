#include "basefinderview.h"
#include "finderitemdelegate.h"
#include "basesqlmodel.h"
#include "model/item.h"
#include "finderwidget.h"

BaseFinderView::BaseFinderView(QWidget *parent) : QListView(parent) {

    playIconHovered = false;

    this->setItemDelegate(new FinderItemDelegate(this));
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->setMouseTracking(true);

    // layout
    this->setGridSize(QSize(151, 151));
    // this->setViewMode(QListView::IconMode);
    // this->setSpacing(1);
    this->setFlow(QListView::LeftToRight);
    this->setWrapping(true);
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
    this->setDragDropMode(QAbstractItemView::DragOnly);

    // cosmetics
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setFrameShape( QFrame::NoFrame );
    this->setAttribute(Qt::WA_MacShowFocusRect, false);

    verticalScrollBar()->setPageStep(3);
    verticalScrollBar()->setSingleStep(1);

}

void BaseFinderView::leaveEvent(QEvent *event) {
    BaseSqlModel *baseModel = static_cast<BaseSqlModel *>(model());
    if (baseModel)
        baseModel->clearHover();
}

void BaseFinderView::mouseMoveEvent(QMouseEvent *event) {
    QListView::mouseMoveEvent(event);

    // BaseSqlModel *baseModel = dynamic_cast<BaseSqlModel *>(model());
    // if (!baseModel) return;

    // qDebug() << "BaseFinderView::mouseMoveEvent" << event->pos();

    if (isHoveringPlayIcon(event)) {
        QMetaObject::invokeMethod(model(), "enterPlayIconHover", Qt::DirectConnection);
        // baseModel->enterPlayIconHover();
        setCursor(Qt::PointingHandCursor);
    } else {
        QMetaObject::invokeMethod(model(), "exitPlayIconHover", Qt::DirectConnection);
        // baseModel->exitPlayIconHover();
        unsetCursor();
    }

    const QModelIndex index = indexAt(event->pos());

    Item *item = index.data(Finder::DataObjectRole).value<ItemPointer>();
    if (item)
        qDebug() << item->getName();

}

void BaseFinderView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton
        && isHoveringPlayIcon(event)) {
        emit play(indexAt(event->pos()));
    } else {
        QListView::mousePressEvent(event);
    }
}

bool BaseFinderView::isHoveringPlayIcon(QMouseEvent *event) {
    const QModelIndex itemIndex = indexAt(event->pos());
    const QRect itemRect = visualRect(itemIndex);

    // qDebug() << " itemRect.x()" <<  itemRect.x();

    const int x = event->x() - itemRect.x();
    const int y = event->y() - itemRect.y();
    return x > 90 && x < 140 && y > 10 && y < 60;
}
