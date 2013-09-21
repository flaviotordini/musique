#include "imagedownloader.h"

#include <QtSql>
#include "database.h"

#include "networkaccess.h"
#include "model/artist.h"
#include "model/album.h"

namespace The {
NetworkAccess* http();
}

class ImageDownload {

public:
    int id;
    int objectId;
    int type;
    int errors;
    QString url;
};

ImageDownloaderThread::ImageDownloaderThread(QObject *parent) : QThread(parent) {
    // This will be used by Database to cache connections for this thread
    setObjectName("imageDownloader" + QString::number(qrand()));
}

void ImageDownloaderThread::run() {
    qDebug() << "Starting image downloads";

    imageDownloader = new ImageDownloader();
    imageDownloader->run();

    // Start thread event loop
    // This makes signals and slots work
    exec();

    Database::instance().closeConnection();
    delete imageDownloader;

    qDebug() << "ImageDownloaderThread::run() exited";
}

ImageDownloader::ImageDownloader(QObject *parent) : QObject(parent), imageDownload(0) {

}

void ImageDownloader::enqueue(int objectId, int objectType, QString url) {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert into downloads (objectid, type, errors, url) values (?,?,0,?)");
    query.bindValue(0, objectId);
    query.bindValue(1, objectType);
    query.bindValue(2, url);

    if (!query.exec())
        qWarning() << query.lastQuery() << query.lastError().text();
}

void ImageDownloader::run() {
    popFromQueue();
}

void ImageDownloader::popFromQueue() {

    // get the next download
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id, objectid, type, errors, url from downloads "
                  "where errors<10 order by type, errors, id limit 1");
    if (!query.exec())
        qWarning() << query.lastQuery() << query.lastError().text();
    if (!query.next()) {
        qDebug() << "Downloads finished";
        thread()->exit();
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
    QObject *reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(imageDownloaded(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SLOT(imageDownloadError(QNetworkReply*)));
}

void ImageDownloader::imageDownloaded(QByteArray bytes) {
    // save image and notify
    if (imageDownload->type == ImageDownloader::ArtistType) {
        Artist* artist = Artist::forId(imageDownload->objectId);
        if (artist) artist->setPhoto(bytes);
        else qDebug() << imageDownload->url << "has no matching artist";
    } else if (imageDownload->type == ImageDownloader::AlbumType) {
        Album* album = Album::forId(imageDownload->objectId);
        if (album) album->setPhoto(bytes);
        else qDebug() << imageDownload->url << "has no matching album";
    } else {
        qDebug() << "Unknown object type" << imageDownload->type;
    }

    // delete row
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("delete from downloads where id=?");
    query.bindValue(0, imageDownload->id);
    if (!query.exec())
        qWarning() << query.lastQuery() << query.lastError().text();

    delete imageDownload;
    imageDownload = 0;

    popFromQueue();
}

void ImageDownloader::imageDownloadError(QNetworkReply *reply) {
    // Increase errorcount
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query = QSqlQuery(db);
    query.prepare("update downloads set errors=errors+1 where id=?");
    query.bindValue(0, imageDownload->id);
    if (!query.exec())
        qWarning() << query.lastQuery() << query.lastError().text();

    delete imageDownload;
    imageDownload = 0;
    popFromQueue();
}
