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

#include "collectionscanner.h"
#include "coverutils.h"
#include "database.h"
#include "datautils.h"
#include "imagedownloader.h"
#include "model/track.h"
#include "tagchecker.h"
#include "tagutils.h"

#include "model/genre.h"

CollectionScanner::CollectionScanner(QObject *parent)
    : QObject(parent), working(false), stopped(false), incremental(false), lastUpdate(0),
      maxQueueSize(0) {
#ifdef APP_MAC
    QString iTunesAlbumArtwork = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
                                 "/iTunes/Album Artwork";
    directoryBlacklist.append(iTunesAlbumArtwork);
#endif

    fileExtensionsBlacklist << "jpg"
                            << "jpeg"
                            << "png"
                            << "gif"
                            << "bmp"
                            << "tif"
                            << "tiff"
                            << "txt"
                            << "doc"
                            << "rtf"
                            << "pdf"
                            << "html"
                            << "htm"
                            << "ps"
                            << "xls"
                            << "js"
                            << "css"
                            << "db"
                            << "log"
                            << "url"
                            << "nfo"
                            << "ini"
                            << "dat"
                            << "md5"
                            << "sfv"
                            << "DS_Store"
                            << "zip"
                            << "rar"
                            << "dmg"
                            << "iso"
                            << "m3u"
                            << "pls"
                            << "cue"
                            << "avi"
                            << "flv"
                            << "mpg"
                            << "wmv"
                            << "swf";
}

void CollectionScanner::reset() {
    stopped = false;
    fileQueue.clear();
    maxQueueSize = 0;
    loadedArtists.clear();
    filesWaitingForArtists.clear();
    loadedAlbums.clear();
    filesWaitingForAlbums.clear();
    processedTrackPaths.clear();
    tracksNeedingFix.clear();
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
        // check whether dir exists, is readable and isn't empty
        // we don't want to wipe out a collection because of an unmounted disk or remote share
        bool proceed = rootDirectory.exists();
        qDebug() << rootDirectory.absolutePath() << "exists" << proceed;

#ifndef APP_MAC_STORE // For some reason this does not work in the sandbox
        // but the dir could be empty
        if (proceed && !QFileInfo(rootDirectory.absolutePath()).isSymLink()) {
            proceed = rootDirectory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries).count() >
                      0;
            qDebug() << "not empty" << proceed;
            if (proceed) {
                proceed = rootDirectory.isReadable();
                qDebug() << "readable" << proceed;
            }
        }
#endif

        if (proceed) {
            // quickly check if anything changed since the last time
            // by building a hash from filenames and mtimes
            QString hash = directoryHash(rootDirectory);
            QSettings settings;
            QString previousHash = settings.value("collectionHash").toString();
            proceed = hash != previousHash;
            qDebug() << "Collection has changed" << proceed;
        }

        if (!proceed) {
            qDebug() << "Not updating collection";
            stopped = false;
            working = false;
            Database::instance().closeConnection();
            QTimer::singleShot(0, this, SLOT(emitFinished()));
            return;
        }

        // get the timestamp of the last db update
        // we'll use it to determine if files have changed since the last time
        lastUpdate = Database::instance().lastUpdate();
        trackPaths = getTrackPaths();
        nontrackPaths = getNonTrackPaths();

    } else {
        // delete any existing data
        Database::instance().clear();

        // invalidate caches
        Artist::clearCache();
        Album::clearCache();
        Track::clearCache();
        Genre::clearCache();
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
    // qDebug() << "Processing " << fileInfo.absoluteFilePath();

    // parse metadata with TagLib
    QString filename = fileInfo.absoluteFilePath();
    Tags *tags = TagUtils::load(filename);

    // if taglib cannot parse the file, drop it
    if (!tags) {
        // qDebug() << "Taglib cannot parse" << fileInfo.absoluteFilePath();
        fileQueue.removeAll(fileInfo);

        // add to nontracks table
        QString path = fileInfo.absoluteFilePath();
        path.remove(this->rootDirectory.absolutePath() + "/");
        insertOrUpdateNonTrack(path, QDateTime::currentSecsSinceEpoch());

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

    /*
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
    */
    file->setTags(tags);

    // get data from the internet
    giveThisFileAnArtist(file);
}

void CollectionScanner::stop() {
    if (working) {
        qDebug() << "Scan stopped";
        Database::instance().getConnection().rollback();
        Database::instance().closeConnection();
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
    Database::instance().setLastUpdate(QDateTime::currentSecsSinceEpoch());

    if (!incremental) {
        if (!Database::instance().getConnection().commit()) {
            qWarning() << "Commit failed!";
        }
    }

    QString hash = directoryHash(rootDirectory);
    QSettings settings;
    settings.setValue("collectionHash", hash);
    qDebug() << "Setting collection hash to" << hash;
    trackPaths.clear();

    QSqlQuery("vacuum", Database::instance().getConnection());

    stopped = false;
    working = false;

    Database::instance().closeConnection();

    qDebug() << "Scan complete";

    QTimer::singleShot(0, this, SLOT(emitFinished()));
}

void CollectionScanner::emitFinished() {
    QVariantMap stats;
    stats.insert("trackCount", processedTrackPaths.size());
    stats.insert("trackPaths", processedTrackPaths);
    stats.insert("tracksNeedingFix", tracksNeedingFix);
    emit finished(stats);
}

void CollectionScanner::setDirectory(const QString &directory) {
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

QString CollectionScanner::directoryHash(const QDir &directory) {
    return treeFingerprint(directory.absolutePath()).toHex();
}

QByteArray CollectionScanner::treeFingerprint(const QString &path) {
    // qDebug() << "Hashing dir" << directory.absolutePath();
    QDir directory(path);
    directory.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    const QFileInfoList list = directory.entryInfoList();

    QCryptographicHash hash(QCryptographicHash::Md5);

    for (const auto &fileInfo : list) {
        hash.addData(fileInfo.fileName().toUtf8());
        if (fileInfo.isDir()) {
            // this is a directory, recurse
            const QString subDirPath = fileInfo.absoluteFilePath();
#ifdef APP_MAC
            if (directoryBlacklist.contains(subDirPath)) {
                qDebug() << "Skipping directory" << subDirPath;
                continue;
            }
#endif
            hash.addData(treeFingerprint(subDirPath));
        } else {
            // this is a file, add to hash
            // qDebug() << "Hashing file" << fileInfo.absolutePath();
            const qint64 lastModified = fileInfo.lastModified().toMSecsSinceEpoch();
            hash.addData(QByteArray::number(lastModified, 16));
        }
    }
    return hash.result();
}

void CollectionScanner::scanDirectory(const QDir &directory) {
    QStack<QDir> stack;
    stack.push(directory);
    while (!stack.empty()) {
        const QDir dir = stack.pop();
        const QFileInfoList flist =
                dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Readable);

        for (const QFileInfo &fileInfo : flist) {
            if (fileInfo.isFile()) {
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

void CollectionScanner::processFile(const QFileInfo &fileInfo) {
    // qDebug() << "FILE:" << fileInfo.absoluteFilePath();

    // 1GB limit
    static const int MAX_FILE_SIZE = 1024 * 1024 * 1024;
    // skip big files
    if (fileInfo.size() > MAX_FILE_SIZE) {
        // qDebug() << "Skipping file:" << fileInfo.absoluteFilePath();
        return;
    }

    // skip UNIX hidden files
    // blacklist image files and other common file extensions
    if (fileInfo.baseName().isEmpty() ||
        fileExtensionsBlacklist.contains(fileInfo.suffix().toLower())) {
        // qDebug() << "Skipping file:" << fileInfo.absoluteFilePath();
        return;
    }

    // TODO check for .cue file

    if (incremental) {
        if (stopped) return;

        const uint lastModified = fileInfo.lastModified().toSecsSinceEpoch();
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
    // qDebug() << __PRETTY_FUNCTION__ << file->getTags()->getFilename();

    const QString artistTag = file->getTags()->getArtistString();

    // try to normalize the artist name to a simpler form
    const QString artistHash = DataUtils::normalizeTag(DataUtils::cleanTag(artistTag));

    if (loadedArtists.contains(artistHash)) {
        // this artist was already encountered
        Artist *artist = loadedArtists.value(artistHash);
        file->setArtist(artist);
        file->setAlbumArtist(artist);

        // qDebug() << "artist loaded giveThisFileAnAlbum" << file->getFileInfo().baseName();
        giveThisFileAnAlbumArtist(file);

    } else if (filesWaitingForArtists.contains(artistHash)) {
        // this artist is already being processed
        // so we need to add ourselves to the list of waiting files
        // qDebug() << "artist being processed" << artistHash << file->getFileInfo().baseName();
        QVector<FileInfo *> files = filesWaitingForArtists.value(artistHash);
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
    const QString artistTag = file->getTags()->getArtistString();
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
    QVector<FileInfo *> files;
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
        artistId = artist->getId();
    } else {
        qDebug() << "Updating artist" << artist->getName();
        artist->update();
    }

    // now that we have an id, let's enqueue the cover image download
    QString imageUrl = artist->property("imageUrl").toString();
    if (!imageUrl.isEmpty())
        ImageDownloader::instance().enqueue(artistId, ImageDownloader::ArtistType, imageUrl);

    const QString hash = artist->property("originalHash").toString();
    loadedArtists.insert(hash, artist);
    // if (hash != artist->getHash())
    loadedArtists.insert(artist->getHash(), artist);

    // continue the processing of blocked files
    QVector<FileInfo *> files = filesWaitingForArtists.take(hash);
    // qDebug() << files.size() << "files were waiting for artist" << artist->getName();
    for (FileInfo *file : qAsConst(files)) {
        file->setArtist(artist);
        file->setAlbumArtist(artist);
        // qDebug() << "ready for album artist" << file->getFileInfo().baseName();
        giveThisFileAnAlbumArtist(file);
    }

    files = filesWaitingForAlbumArtists.take(hash);
    // qDebug() << files.size() << "files were waiting for album artist" << artist->getName();
    for (FileInfo *file : qAsConst(files)) {
        file->setAlbumArtist(artist);
        // qDebug() << "ready for album" << file->getFileInfo().baseName();
        giveThisFileAnAlbum(file);
    }
}

/*** Album Artist ***/

void CollectionScanner::giveThisFileAnAlbumArtist(FileInfo *file) {
    // qDebug() << __PRETTY_FUNCTION__ << file->getTags()->getFilename();

    const QString albumArtistTag = DataUtils::cleanTag(file->getTags()->getAlbumArtist());
    if (albumArtistTag.isEmpty()) {
        giveThisFileAnAlbum(file);
        return;
    }

    const QString artistHash = DataUtils::normalizeTag(albumArtistTag);
    if (loadedArtists.contains(artistHash)) {
        // this artist was already encountered
        file->setAlbumArtist(loadedArtists.value(artistHash));
        giveThisFileAnAlbum(file);

    } else if (filesWaitingForArtists.contains(artistHash) ||
               filesWaitingForAlbumArtists.contains(artistHash)) {
        // this artist is already being processed
        // so we need to add ourselves to the list of waiting files
        // qDebug() << "artist being processed" << artistHash << file->getFileInfo().baseName();
        QVector<FileInfo *> files = filesWaitingForAlbumArtists.value(artistHash);
        files.append(file);
        filesWaitingForAlbumArtists.insert(artistHash, files);
        // qDebug() << "FILES WAITING 4 ARTISTS" <<  artistHash << files << files.count();

    } else {
        // this artist name was never encountered
        // start processing it
        // qDebug() << "new artist processArtsist" << file->getFileInfo().baseName();
        processAlbumArtist(file);
    }
}

void CollectionScanner::processAlbumArtist(FileInfo *file) {
    Artist *artist = new Artist();
    const QString artistTag = file->getTags()->getAlbumArtist();
    artist->setName(DataUtils::cleanTag(artistTag));
    artist->setProperty("originalHash", artist->getHash());

    // qDebug() << "Processing artist:" << artist->getName() << artist->getHash();

    if (filesWaitingForAlbumArtists.contains(artist->getHash())) {
        qDebug() << "ERROR Processing artist multiple times!" << artist->getName();
    }

    if (loadedArtists.contains(artist->getHash())) {
        qDebug() << "ERROR Artist already processed!" << artist->getName();
    }

    // add this file to filesWaitingForArtists
    // this also acts as a lock for other files
    // when the info is ready, all waiting files will be processed
    QVector<FileInfo *> files;
    files.append(file);
    filesWaitingForAlbumArtists.insert(artist->getHash(), files);

    connect(artist, SIGNAL(gotInfo()), SLOT(gotArtistInfo()));
    artist->fetchInfo();
}

/*** Album ***/

void CollectionScanner::giveThisFileAnAlbum(FileInfo *file) {
    const QString albumTag = DataUtils::cleanTag(file->getTags()->getAlbumString());

    // try to normalize the album title to a simpler form
    Artist *artist = file->getAlbumArtist();
    const QString albumHash = Album::getHash(albumTag, artist);
    // qDebug() << __PRETTY_FUNCTION__ << file->getTags()->getFilename() << albumHash;

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
        QVector<FileInfo *> files = filesWaitingForAlbums.value(albumHash);
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
    // qDebug() << __PRETTY_FUNCTION__ << file->getTags()->getFilename();

    Album *album = new Album();
    const QString albumTag = file->getTags()->getAlbumString();
    album->setTitle(DataUtils::cleanTag(albumTag));
    album->setYear(file->getTags()->getYear());

    Artist *artist = file->getAlbumArtist();
    if (!artist) artist = file->getArtist();
    if (artist)
        album->setArtist(artist); // && artist->getId() > 0
    else
        qDebug() << "Album" << album->getTitle() << "lacks an artist";

    if (artist && artist->getId() <= 0)
        qWarning() << "artist id" << artist->getId() << artist->getName();

    // qDebug() << "Processing album:" << album->getTitle() << album->getHash();

    album->setProperty("originalHash", album->getHash());

    // local covers
    const QString imageLocation = album->getImageLocation();
    if (!QFile::exists(imageLocation)) {
        const QString filePath = file->getFileInfo().absolutePath();
        bool localCover = false;
        localCover = CoverUtils::coverFromFile(filePath, album);
        if (!localCover) {
            localCover = CoverUtils::coverFromTags(filePath, album);
            if (localCover) qDebug() << "Found embedded cover for" << filePath;
        }
        if (localCover) album->setProperty("localCover", true);
    }

    if (loadedAlbums.contains(album->getHash())) {
        qDebug() << "ERROR Album already processed!" << album->getTitle() << album->getHash();
        return;
    }
    if (filesWaitingForAlbums.contains(album->getHash())) {
        qDebug() << "ERROR Processing album multiple times!" << album->getTitle()
                 << album->getHash() << file->getFileInfo().baseName();
        return;
    }

    // add this file to filesWaitingForAlbums
    // this also acts as a lock for other files
    // when the info is ready, all waiting files will be processed
    QVector<FileInfo *> files;
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

    const QVector<FileInfo *> files = filesWaitingForAlbums.take(hash);
    loadedAlbums.insert(hash, album);
    // if (hash != album->getHash())
    loadedAlbums.insert(album->getHash(), album);

    int albumId = Album::idForHash(album->getHash());
    album->setId(albumId);
    if (albumId < 0) {
        // qDebug() << "We have a new cool album:" << album->getTitle();
        album->insert();
        albumId = album->getId();
    } else {
        qDebug() << "Updating album" << album->getTitle();
        album->update();
    }

    // now that we have an id, let's enqueue the cover image download
    QString imageUrl = album->property("imageUrl").toString();
    if (!imageUrl.isEmpty())
        ImageDownloader::instance().enqueue(albumId, ImageDownloader::AlbumType, imageUrl);

    // continue the processing of blocked files
    // qDebug() << files.size() << "files were waiting for album" << album->getTitle();
    for (FileInfo *file : files) {
        file->setAlbum(album);
        processTrack(file);
    }
}

/*** Track ***/

void CollectionScanner::processTrack(FileInfo *file) {
    Track *track = new Track();
    QString titleTag = file->getTags()->getTitle();
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
        if (!album->getArtist()) {
            album->setArtist(artist);
        }
    }

    track->setTitle(DataUtils::cleanTag(titleTag));

    // remove collection root path
    QString path = file->getFileInfo().absoluteFilePath();
    path.remove(this->rootDirectory.absolutePath() + "/");
    track->setPath(path);

    track->setNumber(file->getTags()->getTrackNumber());
    track->setDiskNumber(file->getTags()->getDiskNumber());
    track->setDiskCount(file->getTags()->getDiskCount());

    // prefer embedded year tag, since Last.fm release dates are often wrong
    int year = file->getTags()->getYear();
    if (album && year < 1) year = album->getYear();
    if (year < 0) year = 0;
    track->setYear(year);

    track->setLength(file->getTags()->getDuration());

    // if (artist && artist->getId() > 0) {
    // artist = album->getArtist();
    track->setArtist(artist);
    // }

    QString genreTag = file->getTags()->getGenre();
    if (!genreTag.isEmpty()) {
        auto genreNames = genreTag.split(QRegularExpression("([,/-;]| & )"), Qt::SkipEmptyParts);
        for (QString &genreNameRef : genreNames) {
            QString genreName = Genre::cleanGenreName(genreNameRef);
            if (!genreName.isEmpty()) {
                Genre *genre = Genre::maybeCreateByName(genreName);
                if (genre) track->addGenre(genre);
            }
        }
    }

    QString lyricsTag = file->getTags()->getLyrics();
    if (!lyricsTag.isEmpty()) track->setLyrics(lyricsTag);

    // if (album) album->fixTrackTitle(track);

    // qDebug() << "Removing" << file->getFileInfo().baseName() << "from queue";
    if (!fileQueue.removeAll(file->getFileInfo())) {
        qDebug() << "Cannot remove file from queue";
    }

    path = file->getFileInfo().absoluteFilePath();
    processedTrackPaths << path;
    const bool needsFix = TagChecker::checkTags(file->getTags());
    if (needsFix) tracksNeedingFix << path;

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
    if (!success) qWarning() << query.lastError().text();
    while (query.next()) {
        QString path = query.value(0).toString();
        if (!QFile::exists(collectionRoot + path)) {
            qDebug() << "Removing track" << path;
            Track::remove(path);
        }
    }
}

bool CollectionScanner::isNonTrack(const QString &path) {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select count(*) from nontracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qWarning() << query.lastError().text();
    uint tstamp = 0;
    if (query.next()) return query.value(0).toBool();
    return tstamp;
}

bool CollectionScanner::isModifiedNonTrack(const QString &path, uint lastModified) {
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

bool CollectionScanner::insertOrUpdateNonTrack(const QString &path, uint lastModified) {
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
    paths.reserve(query.size());
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
    paths.reserve(query.size());
    while (query.next()) {
        paths << query.value(0).toString();
    }
    return paths;
}
