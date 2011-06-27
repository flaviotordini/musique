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
