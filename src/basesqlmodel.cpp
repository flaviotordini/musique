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
#include "database.h"
#include "model/item.h"
#include "trackmimedata.h"

BaseSqlModel::BaseSqlModel(QObject *parent) : QSqlQueryModel(parent) {}

void BaseSqlModel::setQuery(const QSqlQuery &query) {
    lastQuery = query.lastQuery();
    QSqlQueryModel::setQuery(query);
}

void BaseSqlModel::setQuery(const QString &query, const QSqlDatabase &db) {
    lastQuery = query;
    QSqlQueryModel::setQuery(query, db);
}

void BaseSqlModel::restoreQuery() {
    if (!query().isValid()) setQuery(lastQuery, Database::instance().getConnection());
}

// --- Sturm und drang ---

Qt::DropActions BaseSqlModel::supportedDropActions() const {
    return Qt::CopyAction;
}

Qt::ItemFlags BaseSqlModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return (defaultFlags | Qt::ItemIsDragEnabled);
    } else
        return defaultFlags;
}

QStringList BaseSqlModel::mimeTypes() const {
    QStringList types;
    types << TRACK_MIME;
    return types;
}

QMimeData *BaseSqlModel::mimeData(const QModelIndexList &indexes) const {
    TrackMimeData *mime = new TrackMimeData();

    for (const QModelIndex &index : indexes) {
        Item *item = itemAt(index);
        if (item) {
            // qDebug() << item->getTracks();
            mime->addTracks(item->getTracks());
        }
    }

    return mime;
}
