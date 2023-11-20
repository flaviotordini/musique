#include "decade.h"

#include <QtSql>

#include "../database.h"
#include "album.h"
#include "painterutils.h"
#include "track.h"

Decade::Decade() : startYear(0) {}

QList<Track *> Decade::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    QString sql = QString("select id from tracks where year>=%1 and year<=%2")
                          .arg(QString::number(startYear), QString::number(startYear + 9));
    bool success = query.exec(sql);
    if (!success) qDebug() << query.lastQuery() << query.lastError();
    QList<Track *> tracks;
    tracks.reserve(query.size());
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track *track = Track::forId(trackId);
        tracks << track;
    }

    return tracks;
}

QPixmap Decade::getThumb(int width, int height, qreal pixelRatio) {
    if (pics.isEmpty()) pics = randomPics();
    if (pics.isEmpty()) return thumb;
    if (thumb.isNull() || thumb.devicePixelRatio() != pixelRatio ||
        thumb.width() != width * pixelRatio) {
        thumb = PainterUtils::collage(pics, width, height, pixelRatio);
    }
    return thumb;
}

QList<QPixmap> Decade::randomPics() {
    QList<QPixmap> pics;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select distinct a.id from genreTracks g, tracks t, albums a "
                  "where g.track=t.id and t.album=a.id and t.year>=? and t.year<=? "
                  "order by random() limit 10");
    query.bindValue(0, startYear);
    query.bindValue(1, startYear + 9);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    while (query.next()) {
        auto album = Album::forId(query.value(0).toInt());
        if (album->hasPhoto()) {
            pics << album->getPhoto();
            if (pics.size() == 4) break;
        }
    }
    return pics;
}
