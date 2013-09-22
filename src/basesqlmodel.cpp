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

#include "basesqlmodel.h"
#include "trackmimedata.h"
#include "database.h"

BaseSqlModel::BaseSqlModel(QObject *parent) : QSqlQueryModel(parent) {
    hoveredRow = -1;
    playIconHovered = false;

    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(1000, 0);
    connect(timeLine, SIGNAL(frameChanged(int)), SLOT(updatePlayIcon()));
}

void BaseSqlModel::setHoveredRow(int row) {
    int oldRow = hoveredRow;
    hoveredRow = row;
    emit dataChanged( createIndex( oldRow, 0 ), createIndex( oldRow, columnCount() - 1 ) );
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount() - 1 ) );
}

void BaseSqlModel::clearHover() {
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount() - 1 ) );
    hoveredRow = -1;
    // timeLine->stop();
}

void BaseSqlModel::enterPlayIconHover() {
    if (playIconHovered) return;
    playIconHovered = true;
    if (timeLine->state() != QTimeLine::Running) {
        timeLine->setDirection(QTimeLine::Forward);
        timeLine->start();
    }
}

void BaseSqlModel::exitPlayIconHover() {
    if (!playIconHovered) return;
    playIconHovered = false;
    if (timeLine->state() == QTimeLine::Running) {
        timeLine->stop();
        timeLine->setDirection(QTimeLine::Backward);
        timeLine->start();
    }
    setHoveredRow(hoveredRow);
}

void BaseSqlModel::updatePlayIcon() {
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount() - 1 ) );
}

// --- Sturm und drang ---

Qt::DropActions BaseSqlModel::supportedDropActions() const {
    return Qt::CopyAction;
}

Qt::ItemFlags BaseSqlModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return ( defaultFlags | Qt::ItemIsDragEnabled );
    } else
        return defaultFlags;
}

QStringList BaseSqlModel::mimeTypes() const {
    QStringList types;
    types << TRACK_MIME;
    return types;
}

QMimeData* BaseSqlModel::mimeData( const QModelIndexList &indexes ) const {

    TrackMimeData* mime = new TrackMimeData();

    foreach( const QModelIndex &index, indexes ) {
        Item *item = itemAt(index);
        if (item) {
            // qDebug() << item->getTracks();
            mime->addTracks(item->getTracks());
        }
    }

    return mime;
}
