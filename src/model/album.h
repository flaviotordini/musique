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

#ifndef ALBUM_H
#define ALBUM_H

#include "artist.h"
#include "item.h"
#include "track.h"
#include <QtWidgets>

class Album : public Item {
    Q_OBJECT

public:
    Album();

    // item
    QList<Track *> getTracks();
    QString getStatusTip();

    // properties
    QString getName() { return name; }
    const QString &getTitle() { return name; }
    void setTitle(const QString &title) { this->name = title; }
    int getYear() { return year; }
    void setYear(int year) { this->year = year; }
    static QString getHash(const QString &name, Artist *artist);
    const QString &getHash();
    QString getWikiLocation();
    QString getWiki();

    // relations
    Artist *getArtist() { return artist; }
    void setArtist(Artist *artist) { this->artist = artist; }

    // data access
    static void clearCache() {
        qDeleteAll(cache);
        cache.clear();
        cache.squeeze();
    }
    static Album *forId(int albumId);
    static int idForHash(const QString &name);
    void insert();
    void update();

    QString formattedDuration();

    // internet
    /**
     * Fix album data using Last.fm web services.
     * Will emit gotInfo() when done.
     * This will also emit gotPhoto() when the photo is ready.
     */
    void fetchInfo();

    bool hasPhoto();
    QPixmap getPhoto();
    QPixmap getThumb(int width, int height, qreal pixelRatio);
    void clearPixmapCache() { pixmap = QPixmap(); }

    QString getImageLocation();

    void fixTrackTitle(Track *track);

public slots:
    void setPhoto(const QByteArray &bytes);

signals:
    void gotInfo();
    void gotPhoto();

private slots:
    void fetchLastFmSearch();
    void parseLastFmSearch(const QByteArray &bytes);
    void fetchLastFmInfo();
    void parseLastFmInfo(const QByteArray &bytes);
    void parseLastFmRedirectedName(QNetworkReply *reply);

private:
    QString getBaseLocation();
    QString fixTrackTitleUsingTitle(Track *track, QString newTitle);

    static QHash<int, Album *> cache;

    QString name;
    int year;
    Artist *artist;
    QString mbid;
    QString hash;
    uint listeners;

    QPixmap pixmap;
};

// This is required in order to use QPointer<Album> as a QVariant
typedef QPointer<Album> AlbumPointer;
Q_DECLARE_METATYPE(AlbumPointer)

#endif // ALBUM_H
