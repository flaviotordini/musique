#include "collectionscanner.h"
#include "database.h"
#include "model/track.h"
#include "datautils.h"

CollectionScanner::CollectionScanner(QObject *parent) :
        QObject(parent),
        working(false),
        stopped(false),
        incremental(false),
        lastUpdate(0),
        maxQueueSize(0) {

#ifdef APP_MAC
    QString iTunesAlbumArtwork = QDesktopServices::storageLocation(QDesktopServices::MusicLocation) + "/iTunes/Album Artwork";
    directoryBlacklist.append(iTunesAlbumArtwork);
#endif
}

void CollectionScanner::reset() {
    stopped = false;
    fileQueue.clear();
    maxQueueSize = 0;
    loadedArtists.clear();
    filesWaitingForArtists.clear();
    loadedAlbums.clear();
    filesWaitingForAlbums.clear();
}

void CollectionScanner::run() {

    // qDebug() << "CollectionScanner::run()" << rootDirectory << incremental;

    if (working) {
        emit error(tr("A scanning task is already running"));
        qDebug() << "A scanning task is already running";
        return;
    }
    working = true;
    stopped = false;
    reset();

    if (incremental) {

        // quickly check if anythingchanged since the last time
        // by building a hash from filenames and mtimes
        QString hash = directoryHash(rootDirectory);
        QSettings settings;
        QString previousHash = settings.value("collectionHash").toString();
        // qDebug() << hash << previousHash;
        if (hash == previousHash) {
            // qDebug() << "Nothing changed since the last time";
            stopped = false;
            working = false;
            QTimer::singleShot(0, this, SLOT(emitFinished()));
            return;
        }

        // get the timestamp of the last db update
        // we'll use it to determine if files have changed since the last time
        lastUpdate = Database::instance().lastUpdate();
        trackPaths = getTrackPaths();
        nontrackPaths = getNonTrackPaths();

    } else {

        // drop the previous, if any
        Database::instance().drop();

        // invalidate caches
        Artist::clearCache();
        Album::clearCache();
        Track::clearCache();
    }

    // now scan the files
    scanDirectory(rootDirectory);

    maxQueueSize = fileQueue.size();
    qDebug() << "Going to scan" << maxQueueSize << "files";

    if (stopped) return;

    if (!incremental) {
        Database::instance().closeConnections();
        // Start transaction
        // http://web.utk.edu/~jplyon/sqlite/SQLite_optimization_FAQ.html#transactions
        Database::instance().getConnection().transaction();
    }

    popFromQueue();

    // qDebug() << "CollectionScanner::run() exited";

}

void CollectionScanner::popFromQueue() {
    if (stopped) return;

    if (fileQueue.isEmpty()) {
        complete();
        return;
    }

    QFileInfo fileInfo = fileQueue.first();
    qDebug() << "Processing " << fileInfo.absoluteFilePath();

    // parse metadata with TagLib
    TagLib::FileRef fileref(fileInfo.absoluteFilePath().toUtf8());
    // or maybe QFile::encodeName(p_FilePath).data()

    // if taglib cannot parse the file, drop it
    if (fileref.isNull()) {
        // qDebug() << "Taglib cannot parse" << fileInfo.absoluteFilePath();
        fileQueue.removeAll(fileInfo);

        // add to nontracks table
        QString path = fileInfo.absoluteFilePath();
        path.remove(this->rootDirectory.absolutePath() + "/");
        insertOrUpdateNonTrack(path, QDateTime::currentDateTime().toTime_t());

        QTimer::singleShot(0, this, SLOT(popFromQueue()));
        return;
    }

    // Ok this is an interesting file

    // This object will experience an incredible adventure,
    // facing countless perils and finally reaching its final destination
    FileInfo *file = new FileInfo();
    file->setFileInfo(fileInfo);

    // Copy TagLib::FileRef in our Tags class.
    // TagLib::FileRef keeps files open and we would quickly reach the max open files limit
    Tags *tags = new Tags();
    TagLib::Tag *tag = fileref.tag();
    if (tag) {
        tags->title = Tags::toQString(tag->title());
        tags->artist = Tags::toQString(tag->artist());
        tags->album = Tags::toQString(tag->album());
        tags->track = tag->track();
        tags->year = tag->year();
        TagLib::AudioProperties *audioProperties = fileref.audioProperties();
        if (audioProperties)
            tags->length = audioProperties->length();
    }
    file->setTags(tags);

    // get data from the internet
    giveThisFileAnArtist(file);
}

void CollectionScanner::stop() {
    if (working) {
        qDebug() << "Scan stopped";
        Database::instance().getConnection().rollback();
        stopped = true;
        working = false;
        qDebug() << "stop thread" << thread();
        thread()->exit();
    }
}

void CollectionScanner::complete() {

    if (incremental) {
        // clean db from stale data: non-existing files
        cleanStaleTracks();
    }

    Database::instance().setCollectionRoot(rootDirectory.absolutePath());
    Database::instance().setStatus(ScanComplete);
    Database::instance().setLastUpdate(QDateTime::currentDateTime().toTime_t());

    if (!incremental) {
        if (!Database::instance().getConnection().commit()) {
            qDebug() << "Commit failed!";
        }
    }

    if (incremental) {
        QString hash = directoryHash(rootDirectory);
        QSettings settings;
        settings.setValue("collectionHash", hash);
        qDebug() << "Setting collection hash to" << hash;
        trackPaths.clear();
    }

    stopped = false;
    working = false;


    qDebug() << "Scan complete";

    QTimer::singleShot(0, this, SLOT(emitFinished()));
}

void CollectionScanner::emitFinished() {
    // qDebug() << "emitFinished()";
    emit finished();
}

void CollectionScanner::setDirectory(QString directory) {
    if (working) {
        emit error("A scanning task is already running");
        return;
    }
    if (directory.isEmpty()) {
        incremental = true;
        rootDirectory = Database::instance().collectionRoot();
    } else {
        incremental = false;
        rootDirectory = directory;
    }
}

QString CollectionScanner::directoryHash(QDir directory) {
    QString fingerPrint = treeFingerprint(directory, QString());
    return DataUtils::md5(fingerPrint);
}

QString CollectionScanner::treeFingerprint(QDir directory, QString hash) {
    // qDebug() << "Hashing" << directory.absolutePath();
    directory.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList list = directory.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.isDir()) {
            // this is a directory, recurse
            QString subDirPath = fileInfo.absoluteFilePath();
#ifdef APP_MAC
                if (directoryBlacklist.contains(subDirPath)) {
                    // qDebug() << "Skipping directory" << subDirPath;
                    continue;
                }
#endif
            hash += DataUtils::md5(treeFingerprint(QDir(subDirPath), hash));
        } else {
            // this is a file, add to hash
            const uint lastModified = fileInfo.lastModified().toTime_t();
            hash += QString::number(lastModified);
            hash += fileInfo.absoluteFilePath();
        }
    }
    // qDebug() << hash;
    return hash;
}

void CollectionScanner::scanDirectory(QDir directory) {

    QStack<QDir> stack;
    stack.push(directory);
    while(!stack.empty()) {

        const QDir dir = stack.pop();
        const QFileInfoList flist = dir.entryInfoList(
                QDir::NoDotAndDotDot |
                QDir::Dirs | QDir::Files
                );

        QFileInfo fileInfo;
        Q_FOREACH(fileInfo, flist) {
            if(fileInfo.isFile() ) {
                processFile(fileInfo);
            } else if (fileInfo.isDir()) {
                QString subDirPath = fileInfo.absoluteFilePath();
#ifdef APP_MAC
                if (directoryBlacklist.contains(subDirPath)) {
                    qDebug() << "Skipping directory" << subDirPath;
                    continue;
                }
#endif
                QDir subDir(subDirPath);
                stack.push(subDir);
            }
        }
    }

}

void CollectionScanner::processFile(QFileInfo fileInfo) {
    // qDebug() << "FILE:" << fileInfo.absoluteFilePath();

    // 1GB limit
    static const int MAX_FILE_SIZE = 1024 * 1024 * 1024;
    // skip big files
    if (fileInfo.size() > MAX_FILE_SIZE) {
        qDebug() << "Skipping file:" << fileInfo.absoluteFilePath();
        return;
    }

    // TODO check for .cue file

    if (incremental) {

        // give a break to our poor CPU
        // thread()->msleep(1);

        if (stopped) return;

        const uint lastModified = fileInfo.lastModified().toTime_t();
        if (lastModified > lastUpdate) {
            // qDebug() << "lastModified > lastUpdate" << path;
            qDebug() << "Modified file" << fileInfo.absoluteFilePath();
            fileQueue << fileInfo;
            return;
        }


        /*
        if (Track::isModified(path, lastModified)) {
            // qDebug() << "Track::isModified" << path;
            fileQueue << fileInfo;

        } else {
        */

        if (stopped) return;

        // path relative to the root of the collection
        QString path = fileInfo.absoluteFilePath();
        path.remove(this->rootDirectory.absolutePath() + "/");

        // qDebug() << "Trying !isNonTrack && !isTrack" << path;
        // !isNonTrack(path) &&
        // if (!Track::exists(path)) {
        if (!trackPaths.contains(path) && !nontrackPaths.contains(path)) {
            qDebug() << "New file" << path;
            fileQueue << fileInfo;
        }

        // }

    } else {
        // non-incremental scan, i.e. first scan: scan every file
        fileQueue << fileInfo;
    }

}

/*** Artist ***/

void CollectionScanner::giveThisFileAnArtist(FileInfo *file) {

    const QString artistTag = file->getTags()->artist;

    // try to normalize the artist name to a simpler form
    const QString artistHash = DataUtils::normalizeTag(DataUtils::cleanTag(artistTag));

    if (loadedArtists.contains(artistHash)) {
        // this artist was already encountered
        file->setArtist(loadedArtists.value(artistHash));
        // qDebug() << "artist loaded giveThisFileAnAlbum" << file->getFileInfo().baseName();
        giveThisFileAnAlbum(file);

    } else if (filesWaitingForArtists.contains(artistHash)) {
        // this artist is already being processed
        // so we need to add ourselves to the list of waiting files
        // qDebug() << "artist being processed" << artistHash << file->getFileInfo().baseName();
        QList<FileInfo *> files = filesWaitingForArtists.value(artistHash);
        files.append(file);
        filesWaitingForArtists.insert(artistHash, files);
        // qDebug() << "FILES WAITING 4 ARTISTS" <<  artistHash << files << files.count();

    } else {
        // this artist name was never encountered
        // start processing it
        // qDebug() << "new artist processArtsist" << file->getFileInfo().baseName();
        processArtist(file);
    }
}

void CollectionScanner::processArtist(FileInfo *file) {

    Artist *artist = new Artist();
    const QString artistTag = file->getTags()->artist;
    artist->setName(DataUtils::cleanTag(artistTag));
    artist->setProperty("originalHash", artist->getHash());

    // qDebug() << "Processing artist:" << artist->getName() << artist->getHash();

    if (filesWaitingForArtists.contains(artist->getHash())) {
        qDebug() << "ERROR Processing artist multiple times!" << artist->getName();
    }

    if (loadedArtists.contains(artist->getHash())) {
        qDebug() << "ERROR Artist already processed!" << artist->getName();
    }

    // add this file to filesWaitingForArtists
    // this also acts as a lock for other files
    // when the info is ready, all waiting files will be processed
    QList<FileInfo *> files;
    files.append(file);
    filesWaitingForArtists.insert(artist->getHash(), files);

    connect(artist, SIGNAL(gotInfo()), SLOT(gotArtistInfo()));
    artist->fetchInfo();

}

void CollectionScanner::gotArtistInfo() {

    // get the Artist that sent the signal
    Artist *artist = static_cast<Artist *>(sender());
    if (!artist) {
        qDebug() << "Cannot get sender";
        return;
    }
    // qDebug() << "got info for" << artist->getName();

    int artistId = Artist::idForName(artist->getName());
    if (artistId < 0) {
        // qDebug() << "We have a new promising artist:" << artist->getName();
        artist->insert();
        // TODO last insert id
        artistId = Artist::idForName(artist->getName());
    } else {
        qDebug() << "Updating artist" << artist->getName();
        artist->update();
    }
    artist->setId(artistId);

    const QString hash = artist->property("originalHash").toString();
    QList<FileInfo *> files = filesWaitingForArtists.value(hash);
    filesWaitingForArtists.remove(hash);
    loadedArtists.insert(hash, artist);
    // if (hash != artist->getHash())
    loadedArtists.insert(artist->getHash(), artist);

    // continue the processing of blocked files
    // qDebug() << files.size() << "files were waiting for artist" << artist->getName();
    foreach (FileInfo *file, files) {
        file->setArtist(artist);
        // qDebug() << "ready for album" << file->getFileInfo().baseName();
        giveThisFileAnAlbum(file);
    }

}

/*** Album ***/

void CollectionScanner::giveThisFileAnAlbum(FileInfo *file) {

    const QString albumTag = DataUtils::cleanTag(file->getTags()->album);

    // try to normalize the album title to a simpler form
    const QString albumHash = DataUtils::normalizeTag(albumTag);

    if (albumTag.isEmpty()) {
        processTrack(file);

    } else if (loadedAlbums.contains(albumHash)) {
        // this album was already encountered
        // qDebug() << loadedAlbums.value(albumHash) << "is already loaded";
        file->setAlbum(loadedAlbums.value(albumHash));
        processTrack(file);

    } else if (filesWaitingForAlbums.contains(albumHash)) {
        // this album title is already being processed
        // so we need to add ourselves to the list of waiting files
        // qDebug() << "will wait for album" << albumHash;
        QList<FileInfo *> files = filesWaitingForAlbums.value(albumHash);
        files.append(file);
        filesWaitingForAlbums.insert(albumHash, files);

    } else {
        // this album title was never encountered
        // start processing it
        // qDebug() << "new album processAlbum" << albumHash << filesWaitingForAlbums.keys();
        processAlbum(file);
    }
}

void CollectionScanner::processAlbum(FileInfo *file) {

    Album *album = new Album();
    const QString albumTag = file->getTags()->album;
    album->setTitle(DataUtils::cleanTag(albumTag));
    album->setYear(file->getTags()->year);
    album->setProperty("originalHash", album->getHash());

    Artist *artist = file->getArtist();
    if (artist && artist->getId() > 0) album->setArtist(artist);
    else qDebug() << "Album" << album->getTitle() << "lacks an artist";
    // qDebug() << "Processing album:" << album->getTitle() << album->getHash();

    if (loadedAlbums.contains(album->getHash())) {
        qDebug() << "ERROR Album already processed!" << album->getTitle() << album->getHash();
        return;
    }
    if (filesWaitingForAlbums.contains(album->getHash())) {
        qDebug() << "ERROR Processing album multiple times!"
                << album->getTitle() << album->getHash() << file->getFileInfo().baseName();
        return;
    }

    // add this file to filesWaitingForAlbums
    // this also acts as a lock for other files
    // when the info is ready, all waiting files will be processed
    QList<FileInfo *> files;
    files.append(file);
    filesWaitingForAlbums.insert(album->getHash(), files);

    connect(album, SIGNAL(gotInfo()), SLOT(gotAlbumInfo()));
    album->fetchInfo();

}

void CollectionScanner::gotAlbumInfo() {
    // get the Album that sent the signal
    Album *album = static_cast<Album *>(sender());
    if (!album) {
        qDebug() << "Cannot get sender";
        return;
    }

    const QString hash = album->property("originalHash").toString();
    // qDebug() << "got info for album" << album->getTitle() << hash << album->getHash();

    QList<FileInfo *> files = filesWaitingForAlbums.value(hash);
    filesWaitingForAlbums.remove(hash);
    loadedAlbums.insert(hash, album);
    // if (hash != album->getHash())
    loadedAlbums.insert(album->getHash(), album);

    int albumId = Album::idForName(album->getTitle());
    album->setId(albumId);
    if (albumId < 0) {
        qDebug() << "We have a new cool album:" << album->getTitle();
        album->insert();
        // TODO last insert id
        albumId = Album::idForName(album->getTitle());
    } else {
        qDebug() << "Updating album" << album->getTitle();
        album->update();
    }
    album->setId(albumId);

    // continue the processing of blocked files
    // qDebug() << files.size() << "files were waiting for album" << album->getTitle();
    foreach (FileInfo *file, files) {
        file->setAlbum(album);
        processTrack(file);
    }

}

/*** Track ***/

void CollectionScanner::processTrack(FileInfo *file) {
    Track *track = new Track();
    QString titleTag = file->getTags()->title;
    // qDebug() << "we have a fresh track:" << titleTag;
    if (titleTag.isEmpty()) {
        titleTag = file->getFileInfo().baseName();
        // TODO clean filename:
        // strip constant part of the filenames
        // strip track number
    }

    Artist *artist = file->getArtist();
    if (artist && artist->getId() > 0) track->setArtist(artist);
    // else qDebug() << "track"<< track->getTitle() << "has no artist";

    Album *album = file->getAlbum();
    if (album) {
        if (album->getId() > 0) track->setAlbum(album);
        // else qDebug() << "track"<< track->getTitle() << "has no album";
        if  (!album->getArtist()) {
            album->setArtist(artist);
        }
    }

    track->setTitle(DataUtils::cleanTag(titleTag));

    // remove collection root path
    QString path = file->getFileInfo().absoluteFilePath();
    path.remove(this->rootDirectory.absolutePath() + "/");
    track->setPath(path);

    track->setNumber(file->getTags()->track);

    // prefer embedded year tag, since Last.fm release dates are often wrong
    int year = file->getTags()->year;
    if (album && year < 1)
        year = album->getYear();
    if (year < 0) year = 0;
    track->setYear(year);

    track->setLength(file->getTags()->length);

    // if (artist && artist->getId() > 0) {
    // artist = album->getArtist();
    track->setArtist(artist);
    // }

    // qDebug() << "Removing" << file->getFileInfo().baseName() << "from queue";
    if (!fileQueue.removeAll(file->getFileInfo())) {
        qDebug() << "Cannot remove file from queue";
    }

    connect(track, SIGNAL(gotInfo()), SLOT(gotTrackInfo()));
    track->fetchInfo();

    // http://musicbrainz.org/ws/1/release/579278d5-75dc-4d2f-a5f3-6cc86f6c510e?type=xml&inc=tracks

}

void CollectionScanner::gotTrackInfo() {

    // get the Track that sent the signal
    Track *track = static_cast<Track *>(sender());
    if (!track) {
        qDebug() << "Cannot get sender";
        return;
    }
    // qDebug() << "got info for track" << track->getTitle();

    if (incremental && Track::exists(track->getPath())) {
        qDebug() << "Updating track:" << track->getTitle();
        // qDebug() << "with album" << track->getAlbum() << track->getAlbum()->getId();
        // qDebug() << "with artist" << track->getArtist() << track->getArtist()->getId();
        track->update();
    } else {
        // qDebug() << "We have a new cool track:" << track->getTitle();
        track->insert();
    }

    /*
    qDebug() << "tracks:" << fileQueue.size()
            << "albums:" << filesWaitingForAlbums.size()
            << "artists:" << filesWaitingForArtists.size();
            */

    int percent = (maxQueueSize - fileQueue.size()) * 100 / maxQueueSize;
    emit progress(percent);

    // next!
    QTimer::singleShot(0, this, SLOT(popFromQueue()));

    /*
    else if (fileQueue.size() < 30) {
        qDebug() << "Queue";
        foreach (QFileInfo file, fileQueue) {
            qDebug() << file.filePath();
        }
        qDebug() << "Tracks waiting for albums";
        foreach (QString hash, filesWaitingForAlbums.keys()) {
            qDebug() << hash;
        }
        qDebug() << "Tracks waiting for artists";
        foreach (QString hash, filesWaitingForArtists.keys()) {
            qDebug() << hash;
        }
    }*/

}

void CollectionScanner::cleanStaleTracks() {
    QString collectionRoot = rootDirectory.absolutePath() + "/";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select path from tracks");
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    while (query.next()) {
        QString path = query.value(0).toString();
        if (!QFile::exists(collectionRoot + path)) {
            qDebug() << "Removing track" << path;
            Track::remove(path);
        }
    }
}

bool CollectionScanner::isNonTrack(QString path) {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select count(*) from nontracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    uint tstamp = 0;
    if (query.next())
        return query.value(0).toBool();
    return tstamp;
}

bool CollectionScanner::isModifiedNonTrack(QString path, uint lastModified) {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select tstamp from nontracks where path=? and tstamp<?");
    query.bindValue(0, path);
    query.bindValue(1, lastModified);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    bool ret = query.next();
    qDebug() << path << lastModified << ret;
    return query.next();
}

bool CollectionScanner::insertOrUpdateNonTrack(QString path, uint lastModified) {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert or replace into nontracks (path, tstamp) values (?, ?)");
    query.bindValue(0, path);
    query.bindValue(1, lastModified);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    return !query.next();
}

QStringList CollectionScanner::getTrackPaths() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select path from tracks");
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    QStringList paths;
    while (query.next()) {
        paths << query.value(0).toString();
    }
    return paths;
}

QStringList CollectionScanner::getNonTrackPaths() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select path from nontracks");
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    QStringList paths;
    while (query.next()) {
        paths << query.value(0).toString();
    }
    return paths;
}
