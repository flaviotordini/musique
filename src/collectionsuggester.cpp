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

CollectionSuggester::CollectionSuggester(QObject *parent) : Suggester(parent) {

}

void CollectionSuggester::suggest(const QString &query) {
    QString q = query.simplified();
    if (q.isEmpty()) return;

    QList<Suggestion*> suggestions;

    QString likeQuery;
    if (q.length() < 3) likeQuery = q + "%";
    else likeQuery = "%" + q + "%";

    QSqlDatabase db = Database::instance().getConnection();

    QSqlQuery sqlQuery(db);
    sqlQuery.prepare("select name from artists where name like ? and trackCount>1 order by trackCount desc limit 5");
    sqlQuery.bindValue(0, likeQuery);
    bool success = sqlQuery.exec();
    if (!success) qDebug() << sqlQuery.lastQuery() << sqlQuery.lastError().text() << sqlQuery.lastError().number();
    while (sqlQuery.next()) {
        suggestions << new Suggestion(sqlQuery.value(0).toString(), "artist");
    }

    sqlQuery.prepare("select title from albums where (title like ? or year=?) and trackCount>0 order by year desc, trackCount desc limit 5");
    sqlQuery.bindValue(0, likeQuery);
    sqlQuery.bindValue(1, q);
    success = sqlQuery.exec();
    if (!success) qDebug() << sqlQuery.lastQuery() << sqlQuery.lastError().text() << sqlQuery.lastError().number();
    while (sqlQuery.next()) {
        suggestions << new Suggestion(sqlQuery.value(0).toString(), "album");
    }

    sqlQuery.prepare("select title from tracks where title like ? order by track, path limit 5");
    sqlQuery.bindValue(0, likeQuery);
    success = sqlQuery.exec();
    if (!success) qDebug() << sqlQuery.lastQuery() << sqlQuery.lastError().text() << sqlQuery.lastError().number();
    while (sqlQuery.next()) {
        suggestions << new Suggestion(sqlQuery.value(0).toString(), "track");
    }

    emit ready(suggestions);
}
