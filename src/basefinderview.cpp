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
    delegate = new FinderItemDelegate(this);
    setItemDelegate(delegate);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // layout
    // setGridSize(QSize(FinderItemDelegate::ITEM_WIDTH + 1, FinderItemDelegate::ITEM_HEIGHT + 1));
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    // setUniformItemSizes(true);

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

    hoveredRow = -1;
    playIconHovered = false;
    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(0, 100);
    timeLine->setCurveShape(QTimeLine::LinearCurve);
    connect(timeLine, SIGNAL(frameChanged(int)), SLOT(updatePlayIcon()));
    connect(this, SIGNAL(entered(const QModelIndex &)), SLOT(setHoveredIndex(const QModelIndex &)));
    connect(this, SIGNAL(viewportEntered()), SLOT(clearHover()));
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

void BaseFinderView::setHoveredIndex(const QModelIndex &index) {
    setHoveredRow(index.row());
}

void BaseFinderView::setHoveredRow(int row) {
    int oldRow = hoveredRow;
    hoveredRow = row;
    refreshRow(oldRow);
    refreshRow(hoveredRow);
}

void BaseFinderView::clearHover() {
    refreshRow(hoveredRow);
    hoveredRow = -1;
}

void BaseFinderView::enterPlayIconHover() {
    if (playIconHovered) return;
    playIconHovered = true;
    if (timeLine->state() != QTimeLine::Running) {
        timeLine->setDirection(QTimeLine::Forward);
        timeLine->start();
    }
}

void BaseFinderView::exitPlayIconHover() {
    if (!playIconHovered) return;
    playIconHovered = false;
    if (timeLine->state() == QTimeLine::Running) {
        timeLine->stop();
        timeLine->setDirection(QTimeLine::Backward);
        timeLine->start();
    }
    setHoveredRow(hoveredRow);
}

void BaseFinderView::refreshIndex(const QModelIndex &index) {
    bool res = QMetaObject::invokeMethod(model(), "refreshIndex", Qt::DirectConnection,
                                         Q_ARG(QModelIndex, index));
    if (!res) {
        qDebug() << "Cannot invoke refreshIndex on " << model() << "for index" << index;
    }
}

void BaseFinderView::refreshRow(int row) {
    QModelIndex index = model()->index(row, 0);
    refreshIndex(index);
}

void BaseFinderView::updatePlayIcon() {
    refreshRow(hoveredRow);
}

void BaseFinderView::leaveEvent(QEvent * /* event */) {
    clearHover();
}

void BaseFinderView::mouseMoveEvent(QMouseEvent *event) {
    QListView::mouseMoveEvent(event);

    if (isHoveringPlayIcon(event)) {
        enterPlayIconHover();
        setCursor(Qt::PointingHandCursor);
    } else {
        exitPlayIconHover();
        unsetCursor();
    }
}

void BaseFinderView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isHoveringPlayIcon(event)) {
        emit play(indexAt(event->pos()));
    } else {
        QListView::mouseReleaseEvent(event);
    }
}

void BaseFinderView::resizeEvent(QResizeEvent *event) {
    int width = contentsRect().width() - style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 1;
    int size = qFloor(width / qFloor(width / FinderItemDelegate::ITEM_WIDTH));
    delegate->setItemSize(size - 1, size - 1);
    setGridSize(QSize(size, size));
}

bool BaseFinderView::isHoveringPlayIcon(QMouseEvent *event) {
    const QModelIndex itemIndex = indexAt(event->pos());
    const QRect itemRect = visualRect(itemIndex);

    // qDebug() << " itemRect.x()" <<  itemRect.x();

    const int x = event->x() - itemRect.x();
    const int y = event->y() - itemRect.y();
    const int itemWidth = delegate->getItemWidth();
    return x > itemWidth - 60 && y < 60;
}
