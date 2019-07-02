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

#include "imagedownloader.h"

#include "database.h"
#include <QtSql>

#include "http.h"
#include "httputils.h"
#include "model/album.h"
#include "model/artist.h"

class ImageDownload {
public:
    int id;
    int objectId;
    int type;
    int errors;
    QString url;
};

ImageDownloader::ImageDownloader(QObject *parent) : QObject(parent) {}

ImageDownloader &ImageDownloader::instance() {
    static QMap<QThread *, ImageDownloader *> instances;
    QThread *currentThread = QThread::currentThread();
    auto i = instances.constFind(currentThread);
    if (i != instances.constEnd()) return *i.value();
    ImageDownloader *instance = new ImageDownloader();
    instances.insert(currentThread, instance);
    connect(currentThread, &QThread::destroyed, instance,
            [currentThread] { instances.remove(currentThread); });
    return *instance;
}

void ImageDownloader::enqueue(int objectId, int objectType, const QString &url) {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert into downloads (objectid, type, errors, url) values (?,?,0,?)");
    query.bindValue(0, objectId);
    query.bindValue(1, objectType);
    query.bindValue(2, url);
    if (!query.exec()) qWarning() << query.lastQuery() << query.lastError().text();

    if (!running) QTimer::singleShot(0, this, SLOT(popFromQueue()));
}

void ImageDownloader::start() {
    if (!running) QTimer::singleShot(0, this, SLOT(popFromQueue()));
}

void ImageDownloader::popFromQueue() {
    running = true;

    // get the next download
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id, objectid, type, errors, url from downloads "
                  "where errors<10 order by errors, type, id limit 1");
    if (!query.exec()) qWarning() << query.lastQuery() << query.lastError().text();
    if (!query.next()) {
        qDebug() << "Downloads finished";
        running = false;
        emit finished();
        return;
    }
    if (imageDownload) delete imageDownload;
    imageDownload = new ImageDownload();
    imageDownload->id = query.value(0).toInt();
    imageDownload->objectId = query.value(1).toInt();
    imageDownload->type = query.value(2).toInt();
    imageDownload->errors = query.value(3).toInt();
    imageDownload->url = query.value(4).toString();

    // start download
    QUrl url(imageDownload->url);
    QObject *reply = HttpUtils::notCached().get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(onImageDownloaded(QByteArray)));
    connect(reply, SIGNAL(error(QString)), SLOT(imageDownloadError()));
}

void ImageDownloader::onImageDownloaded(const QByteArray &bytes) {
    if (!imageDownload) {
        qWarning() << "imageDownload is null";
        return;
    }

    // save image and notify
    if (imageDownload->type == ImageDownloader::ArtistType) {
        Artist *artist = Artist::forId(imageDownload->objectId);
        if (artist) {
            if (artist->thread() == thread())
                artist->setPhoto(bytes);
            else
                QMetaObject::invokeMethod(artist, "setPhoto", Q_ARG(QByteArray, bytes));
        } else
            qDebug() << imageDownload->url << "has no matching artist";
    } else if (imageDownload->type == ImageDownloader::AlbumType) {
        Album *album = Album::forId(imageDownload->objectId);
        if (album) {
            if (album->thread() == thread())
                album->setPhoto(bytes);
            else
                QMetaObject::invokeMethod(album, "setPhoto", Q_ARG(QByteArray, bytes));
        } else
            qDebug() << imageDownload->url << "has no matching album";
    } else {
        qDebug() << "Unknown object type" << imageDownload->type;
    }

    // delete row
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("delete from downloads where id=?");
    query.bindValue(0, imageDownload->id);
    if (!query.exec()) qWarning() << query.lastQuery() << query.lastError().text();

    delete imageDownload;
    imageDownload = nullptr;

    QTimer::singleShot(0, this, SLOT(popFromQueue()));
}

void ImageDownloader::imageDownloadError() {
    qWarning() << imageDownload->url;

    // Increase errorcount
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query = QSqlQuery(db);
    query.prepare("update downloads set errors=errors+1 where id=?");
    query.bindValue(0, imageDownload->id);
    if (!query.exec()) qWarning() << query.lastQuery() << query.lastError().text();

    delete imageDownload;
    imageDownload = nullptr;
    popFromQueue();
}
