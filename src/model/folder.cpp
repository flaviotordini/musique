#include "folder.h"

#include <QDesktopServices>
#include "album.h"
#include "artist.h"

#include <QtSql>
#include "../database.h"

static QHash<QString, Folder*> folderCache;

Folder::Folder(QString path, QObject *parent)
    : Item(parent),
    path(path) {
    dir.setPath(path);
    photoLoaded = false;
}

Folder* Folder::forPath(QString path) {

    qDebug() << "Folder::forPath" << path;

    /*
    if (path.startsWith("/")) {
        QSettings settings;
        QString collectionRoot = settings.value("collectionRoot").toString()  + "/";
        path = path.replace(collectionRoot, "");
        qDebug() << "path is now" << path;
    }*/

    if (folderCache.contains(path)) {
        // get from cache
        // qDebug() << "Folder was cached" << path;
        return folderCache.value(path);
    }

    Folder* folder = new Folder(path);
    folderCache.insert(path, folder);

    return folder;
}

Album* Folder::getAlbum() {
    QSqlDatabase db = Database::instance().getConnection();

    QSqlQuery query(db);
    query.prepare("select id from albums where title like ? limit 1");
    query.bindValue(0, "%" + dir.dirName() + "%");
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        return Album::forId(query.value(0).toInt());
    }
    return 0;
}

Artist* Folder::getArtist() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select artist from tracks where path like ? limit 1");
    query.bindValue(0, path + "/%");
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        return Artist::forId(query.value(0).toInt());
    }
    return 0;
}

QImage Folder::getPhoto() {
    if (!photoLoaded) {
        photoLoaded = true;
        // qDebug() << "loading photo for" << path;
        
        // Try to get a relevant Album
        Album *album = getAlbum();
        
        // If there's no album, try to get a relevant Artist
        Artist *artist = 0;
        if (!album) {
            artist = getArtist();
        }

        QString imageLocation;
        QString dataLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
        if (album) {
            imageLocation = dataLocation + "/albums/" + album->getHash();
        } else if (artist) {
            imageLocation = dataLocation + "/artists/" + artist->getHash();
        }

        if (!imageLocation.isEmpty())
            photo = QImage(imageLocation);
    }
    return photo;
}

QList<Track*> Folder::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where path like ? order by album, track, title");
    QSettings settings;
    QString collectionRoot = settings.value("collectionRoot").toString()  + "/";
    QString relativePath = path.replace(collectionRoot, "");
    query.bindValue(0, relativePath + "/%");
    bool success = query.exec();
    //if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    QList<Track*> tracks;
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track* track = Track::forId(trackId);
        tracks << track;
    }
    return tracks;
}
