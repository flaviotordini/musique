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

#include "collectionsuggester.h"

#include <QtSql>
#include "database.h"

CollectionSuggester::CollectionSuggester(QObject *parent) {

}

void CollectionSuggester::suggest(QString q) {
    q = q.simplified();
    if (q.isEmpty()) return;

    QStringList suggestions;

    QString likeQuery;
    if (q.length() < 3) likeQuery = q + "%";
    else likeQuery = "%" + q + "%";

    QSqlDatabase db = Database::instance().getConnection();

    QSqlQuery query(db);
    query.prepare("select name from artists where name like ? and trackCount>1 order by trackCount desc limit 5");
    query.bindValue(0, likeQuery);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    while (query.next()) {
        suggestions << query.value(0).toString();
    }

    query.prepare("select title from albums where (title like ? or year=?) and trackCount>0 order by year desc, trackCount desc limit 5");
    query.bindValue(0, likeQuery);
    query.bindValue(1, q);
    success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    while (query.next()) {
        suggestions << query.value(0).toString();
    }

    query.prepare("select title from tracks where title like ? order by track, path limit 5");
    query.bindValue(0, likeQuery);
    success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    while (query.next()) {
        suggestions << query.value(0).toString();
    }

    suggestions.removeDuplicates();

    emit ready(suggestions);
}
