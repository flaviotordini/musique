#include "decade.h"

#include <QtSql>

#include "../database.h"

#include "album.h"
#include "track.h"

Decade::Decade() : startYear(0), pixmapAlbum(nullptr) {}

QVector<Track *> Decade::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    QString sql = QString("select id from tracks where year>=%1 and year<=%2")
                          .arg(QString::number(startYear), QString::number(startYear + 9));
    bool success = query.exec(sql);
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    QVector<Track *> tracks;
    tracks.reserve(query.size());
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track *track = Track::forId(trackId);
        tracks << track;
    }

    return tracks;
}

QPixmap Decade::getThumb(int width, int height, qreal pixelRatio) {
    if (!pixmapAlbum) pixmapAlbum = randomAlbum();
    if (!pixmapAlbum) return pixmap;
    if (pixmap.isNull() || pixmap.devicePixelRatio() != pixelRatio ||
        pixmap.width() != width * pixelRatio) {
        pixmap = pixmapAlbum->getPhoto();
        if (pixmap.isNull()) return pixmap;

        const int pixelWidth = width * pixelRatio;
        const int pixelHeight = height * pixelRatio;
        const int wDiff = pixmap.width() - pixelWidth;
        const int hDiff = pixmap.height() - pixelHeight;
        if (wDiff || hDiff) {
            pixmap = pixmap.scaled(pixelWidth, pixelHeight, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }
        pixmap.setDevicePixelRatio(pixelRatio);
    }
    return pixmap;
}

Album *Decade::randomAlbum() {
    Album *album = nullptr;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select a.id from genreTracks g, tracks t, albums a "
                  "where g.track=t.id and t.album=a.id and t.year>=? and t.year<=? "
                  "order by random() limit 10");
    query.bindValue(0, startYear);
    query.bindValue(1, startYear + 9);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    while (query.next()) {
        album = Album::forId(query.value(0).toInt());
        if (album->hasPhoto()) break;
    }
    return album;
}
