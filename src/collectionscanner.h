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

#ifndef COLLECTIONSCANNER_H
#define COLLECTIONSCANNER_H

#include "model/album.h"
#include "model/artist.h"
#include <QtCore>

// TagLib
#include "fileref.h"
#include "tag.h"

#include "tags.h"

class FileInfo {
public:
    FileInfo() : artist(0), albumArtist(0), album(0), tags(0) {}
    ~FileInfo() {
        if (artist) delete artist;
        if (album) delete album;
        if (tags) delete tags;
    }
    Tags *getTags() { return tags; }
    void setTags(Tags *tags) { this->tags = tags; }
    Artist *getArtist() { return artist; }
    void setArtist(Artist *artist) { this->artist = artist; }
    Artist *getAlbumArtist() { return albumArtist; }
    void setAlbumArtist(Artist *artist) { albumArtist = artist; }
    Album *getAlbum() { return album; }
    void setAlbum(Album *album) { this->album = album; }
    QFileInfo getFileInfo() { return fileInfo; }
    void setFileInfo(QFileInfo fileInfo) { this->fileInfo = fileInfo; }

private:
    Artist *artist;
    Artist *albumArtist;
    Album *album;
    Tags *tags;
    QFileInfo fileInfo;
};

class CollectionScanner : public QObject {
    Q_OBJECT

public:
    CollectionScanner(QObject *parent);
    void setDirectory(const QString &directory);
    void run();
    void stop();
    void complete();

signals:
    void progress(int);
    void finished(const QVariantMap &stats);
    void error(QString message);

private slots:
    void scanDirectory(const QDir &directory);
    void popFromQueue();
    void giveThisFileAnArtist(FileInfo *file);
    void processArtist(FileInfo *file);
    void gotArtistInfo();
    void giveThisFileAnAlbumArtist(FileInfo *file);
    void processAlbumArtist(FileInfo *file);
    void giveThisFileAnAlbum(FileInfo *file);
    void processAlbum(FileInfo *file);
    void gotAlbumInfo();
    void processTrack(FileInfo *file);
    void emitFinished();

private:
    void reset();
    void processFile(const QFileInfo &fileInfo);
    void cleanStaleTracks();
    static bool isNonTrack(const QString &path);
    static bool isModifiedNonTrack(const QString &path, uint lastModified);
    static bool insertOrUpdateNonTrack(const QString &path, uint lastModified);
    QString directoryHash(const QDir &directory);
    QByteArray treeFingerprint(const QString &path);
    QStringList getTrackPaths();
    QStringList getNonTrackPaths();

    bool working;
    bool stopped;
    bool incremental;
    QDir rootDirectory;
    uint lastUpdate;

    QVector<QFileInfo> fileQueue;
    int maxQueueSize;
    QHash<QString, Artist *> loadedArtists;
    QHash<QString, QVector<FileInfo *>> filesWaitingForArtists;
    QHash<QString, QVector<FileInfo *>> filesWaitingForAlbumArtists;
    QHash<QString, Album *> loadedAlbums;
    QHash<QString, QVector<FileInfo *>> filesWaitingForAlbums;
    QStringList trackPaths;
    QStringList nontrackPaths;

    QStringList directoryBlacklist;
    QStringList fileExtensionsBlacklist;

    QStringList processedTrackPaths;
    QStringList tracksNeedingFix;
};

#endif // COLLECTIONSCANNER_H
