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
    setUniformItemSizes(true);

    // dragndrop
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    // cosmetics
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    verticalScrollBar()->setPageStep(3);

    hoveredRow = -1;
    playIconHovered = false;
    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(0, 100);
    timeLine->setEasingCurve(QEasingCurve::Linear);
    connect(timeLine, &QTimeLine::frameChanged, this, &FinderListView::updatePlayIcon);
    connect(this, &QAbstractItemView::entered, this, &FinderListView::setHoveredIndex);
    connect(this, &QAbstractItemView::viewportEntered, this, &FinderListView::clearHover);

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
    QListView::resizeEvent(event);
}

void FinderListView::keyPressEvent(QKeyEvent *event) {
    const auto key = event->key();
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_P) {
        emit play(currentIndex());
    } else
        QListView::keyPressEvent(event);
}

bool FinderListView::isHoveringPlayIcon(QMouseEvent *event) {
    QModelIndex itemIndex = indexAt(event->pos());
    QRect itemRect = visualRect(itemIndex);
    int x = event->pos().x() - itemRect.x();
    int y = event->pos().y() - itemRect.y();
    int itemWidth = itemRect.width();
    int margin = itemWidth / 8;
    int playIconSize = 50;
    int areaSize = margin + playIconSize;
    return x > itemWidth - areaSize && x < itemWidth && y < areaSize;
}

void FinderListView::updateItemSize() {
    int scrollbarWidth = 0;
    bool transient = style()->styleHint(QStyle::SH_ScrollBar_Transient, 0, verticalScrollBar());
    if (!transient) scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int viewWidth = contentsRect().width() - scrollbarWidth - 1;

    int cols = qFloor(viewWidth / defaultItemWidth);
    int w = cols > 0 ? qFloor(viewWidth / cols) : defaultItemWidth;
    qDebug() << "viewWidth" << viewWidth << "cols" << cols << "w" << w;

    // limit item size to available image resolution
    // const int maxItemWidth = 300;
    // const qreal pixelRatio = viewport()->devicePixelRatioF();
    // if (pixelRatio > 1.0 && w * pixelRatio > maxItemWidth) w = maxItemWidth / pixelRatio;

    int h = w * itemHeightRatio;
    delegate->setItemSize(w, h);
}
