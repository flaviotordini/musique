/* $BEGIN_LICENSE

This file is part of Musique.
Copyright 2013, Flavio Tordini <flavio.tordini@gmail.com>

Musique is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Musique is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Musique.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#include "basefinderview.h"
#include "basesqlmodel.h"
#include "database.h"
#include "finderitemdelegate.h"
#include "finderwidget.h"
#include "model/item.h"

BaseFinderView::BaseFinderView(QWidget *parent) : QListView(parent) {
    setItemDelegate(new FinderItemDelegate(this));
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // layout
    setGridSize(QSize(FinderItemDelegate::ITEM_WIDTH + 1, FinderItemDelegate::ITEM_HEIGHT + 1));
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setUniformItemSizes(true);

    // colors
    QPalette p = palette();
    p.setBrush(QPalette::Base, Qt::black);
    p.setBrush(QPalette::Text, Qt::white);
    p.setColor(QPalette::Background, Qt::transparent);
    setPalette(p);

    // dragndrop
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    // cosmetics
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    verticalScrollBar()->setPageStep(3);

    setAttribute(Qt::WA_OpaquePaintEvent);
}

void BaseFinderView::appear() {
    setEnabled(true);
    setMouseTracking(true);
    BaseSqlModel *baseSqlModel = qobject_cast<BaseSqlModel *>(model());
    if (baseSqlModel) {
        baseSqlModel->restoreQuery();
        while (baseSqlModel->canFetchMore())
            baseSqlModel->fetchMore();
    }
}

void BaseFinderView::disappear() {
    setEnabled(false);
    setMouseTracking(false);
    BaseSqlModel *baseSqlModel = qobject_cast<BaseSqlModel *>(model());
    if (baseSqlModel) baseSqlModel->clear();
}

void BaseFinderView::leaveEvent(QEvent * /* event */) {
    QMetaObject::invokeMethod(model(), "clearHover");
}

void BaseFinderView::mouseMoveEvent(QMouseEvent *event) {
    QListView::mouseMoveEvent(event);

    // qDebug() << "BaseFinderView::mouseMoveEvent" << event->pos();

    if (isHoveringPlayIcon(event)) {
        QMetaObject::invokeMethod(model(), "enterPlayIconHover");
        setCursor(Qt::PointingHandCursor);
    } else {
        QMetaObject::invokeMethod(model(), "exitPlayIconHover");
        unsetCursor();
    }

    /*
    const QModelIndex index = indexAt(event->pos());
    Item *item = index.data(Finder::DataObjectRole).value<ItemPointer>();
    if (item)
        qDebug() << item->getName();
        */
}

void BaseFinderView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isHoveringPlayIcon(event)) {
        emit play(indexAt(event->pos()));
    } else {
        QListView::mouseReleaseEvent(event);
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
