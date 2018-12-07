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

#include "finderlistview.h"
#include "basesqlmodel.h"
#include "database.h"
#include "finderitemdelegate.h"
#include "finderwidget.h"
#include "model/item.h"

FinderListView::FinderListView(QWidget *parent) : QListView(parent) {
    delegate = new FinderItemDelegate(this);
    setItemDelegate(delegate);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // layout
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setUniformItemSizes(true);

    // fix palette propagation
    setPalette(parent->palette());

    // dragndrop
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    // cosmetics
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    verticalScrollBar()->setPageStep(3);

    setAttribute(Qt::WA_OpaquePaintEvent);

    hoveredRow = -1;
    playIconHovered = false;
    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(0, 100);
    timeLine->setCurveShape(QTimeLine::LinearCurve);
    connect(timeLine, SIGNAL(frameChanged(int)), SLOT(updatePlayIcon()));
    connect(this, SIGNAL(entered(const QModelIndex &)), SLOT(setHoveredIndex(const QModelIndex &)));
    connect(this, SIGNAL(viewportEntered()), SLOT(clearHover()));

    modelIsResetting = false;
}

void FinderListView::setModel(QAbstractItemModel *model) {
    connect(model, &QAbstractItemModel::modelAboutToBeReset, this,
            [this] { modelIsResetting = true; });
    connect(model, &QAbstractItemModel::modelReset, this, [this] { modelIsResetting = false; });
    QListView::setModel(model);
}

void FinderListView::appear() {
    setEnabled(true);
    setMouseTracking(true);
    BaseSqlModel *baseSqlModel = qobject_cast<BaseSqlModel *>(model());
    if (baseSqlModel) {
        baseSqlModel->restoreQuery();
        while (baseSqlModel->canFetchMore())
            baseSqlModel->fetchMore();
    }
    updateItemSize();
}

void FinderListView::disappear() {
    setEnabled(false);
    setMouseTracking(false);
    BaseSqlModel *baseSqlModel = qobject_cast<BaseSqlModel *>(model());
    if (baseSqlModel) baseSqlModel->clear();
}

void FinderListView::setHoveredIndex(const QModelIndex &index) {
    setHoveredRow(index.row());
}

void FinderListView::setHoveredRow(int row) {
    int oldRow = hoveredRow;
    hoveredRow = row;
    refreshRow(oldRow);
    refreshRow(hoveredRow);
}

void FinderListView::clearHover() {
    refreshRow(hoveredRow);
    hoveredRow = -1;
}

void FinderListView::enterPlayIconHover() {
    if (playIconHovered) return;
    playIconHovered = true;
    if (timeLine->state() != QTimeLine::Running) {
        timeLine->setDirection(QTimeLine::Forward);
        timeLine->start();
    }
}

void FinderListView::exitPlayIconHover() {
    if (!playIconHovered) return;
    playIconHovered = false;
    if (timeLine->state() == QTimeLine::Running) {
        timeLine->stop();
        timeLine->setDirection(QTimeLine::Backward);
        timeLine->start();
    }
    setHoveredRow(hoveredRow);
}

void FinderListView::refreshIndex(const QModelIndex &index) {
    bool res = QMetaObject::invokeMethod(model(), "refreshIndex", Qt::DirectConnection,
                                         Q_ARG(QModelIndex, index));
    if (!res) {
        qDebug() << "Cannot invoke refreshIndex on " << model() << "for index" << index;
    }
}

void FinderListView::refreshRow(int row) {
    QModelIndex index = model()->index(row, 0);
    refreshIndex(index);
}

void FinderListView::updatePlayIcon() {
    refreshRow(hoveredRow);
}

void FinderListView::leaveEvent(QEvent * /* event */) {
    clearHover();
}

void FinderListView::mouseMoveEvent(QMouseEvent *event) {
    QListView::mouseMoveEvent(event);
    if (modelIsResetting) return;

    if (isHoveringPlayIcon(event)) {
        enterPlayIconHover();
        setCursor(Qt::PointingHandCursor);
    } else {
        exitPlayIconHover();
        unsetCursor();
    }
}

void FinderListView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isHoveringPlayIcon(event)) {
        emit play(indexAt(event->pos()));
    } else {
        QListView::mouseReleaseEvent(event);
    }
}

void FinderListView::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    updateItemSize();
}

bool FinderListView::isHoveringPlayIcon(QMouseEvent *event) {
    const QModelIndex itemIndex = indexAt(event->pos());
    const QRect itemRect = visualRect(itemIndex);

    // qDebug() << " itemRect.x()" <<  itemRect.x();

    const int x = event->x() - itemRect.x();
    const int y = event->y() - itemRect.y();
    const int itemWidth = delegate->getItemWidth();
    return x > itemWidth - 60 && y < 60;
}

void FinderListView::updateItemSize() {
    const qreal pixelRatio = viewport()->devicePixelRatioF();
    const int maxPhotoWidth = 300;
    int size = 0;

    const int itemCount = model()->rowCount(rootIndex());
    if (itemCount > 0 && itemCount < 7) {
        size = maxPhotoWidth / pixelRatio;
    } else {
        int scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        int width = contentsRect().width() - scrollbarWidth - 1;
        int idealItemWidth = qFloor(width / FinderItemDelegate::ITEM_WIDTH);
        size = idealItemWidth > 0 ? qFloor(width / idealItemWidth) : FinderItemDelegate::ITEM_WIDTH;

        // limit item size to available image resolution
        if (pixelRatio > 1.0 && size * pixelRatio > maxPhotoWidth)
            size = maxPhotoWidth / pixelRatio;
    }

    delegate->setItemSize(size, size);
    setGridSize(QSize(size, size));
}
