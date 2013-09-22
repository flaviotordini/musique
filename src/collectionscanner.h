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

#include <QtCore>
#include "model/artist.h"
#include "model/album.h"

// TagLib
#include "fileref.h"
#include "tag.h"

class Tags {

public:
    Tags() : track(0), year(0), length(0) { }
    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    int length;

    static QString toQString(TagLib::String tstring) {
        if (!tstring.isEmpty()) {
            // return QString::fromStdString(tstring.to8Bit(true));
            return QString::fromUtf8(tstring.toCString(true));
        } else return QString();
    }

};

class FileInfo {

public:
    FileInfo() : artist(0), album(0), tags(0) { };
    ~FileInfo() {
        if (artist) delete artist;
        if (album) delete album;
        if (tags) delete tags;
    }
    Tags* getTags()  { return tags; }
    void setTags(Tags* tags) { this->tags = tags; }
    Artist* getArtist()  { return artist; }
    void setArtist(Artist *artist) { this->artist = artist; }
    Album* getAlbum()  { return album; }
    void setAlbum(Album *album) { this->album = album; }
    QFileInfo getFileInfo()  { return fileInfo; }
    void setFileInfo(QFileInfo fileInfo) { this->fileInfo = fileInfo; }

private:
    Artist *artist;
    Album *album;
    Tags *tags;
    QFileInfo fileInfo;

};

class CollectionScanner : public QObject {

    Q_OBJECT

public:
    CollectionScanner(QObject *parent);
    void setDirectory(QString directory);
    void run();
    void stop();
    void complete();

signals:
    void progress(int);
    void finished();
    void error(QString message);

private slots:
    void scanDirectory(QDir directory);
    void popFromQueue();
    void giveThisFileAnArtist(FileInfo *file);
    void processArtist(FileInfo *file);
    void gotArtistInfo();
    void giveThisFileAnAlbum(FileInfo *file);
    void processAlbum(FileInfo *file);
    void gotAlbumInfo();
    void processTrack(FileInfo *file);
    void emitFinished();

private:
    void reset();
    void processFile(QFileInfo fileInfo);
    void cleanStaleTracks();
    static bool isNonTrack(QString path);
    static bool isModifiedNonTrack(QString path, uint lastModified);
    static bool insertOrUpdateNonTrack(QString path, uint lastModified);
    QString directoryHash(QDir directory);
    QString treeFingerprint(QDir directory, QString hash);
    QStringList getTrackPaths();
    QStringList getNonTrackPaths();

    bool working;
    bool stopped;
    bool incremental;
    QDir rootDirectory;
    uint lastUpdate;

    QList<QFileInfo> fileQueue;
    int maxQueueSize;
    QHash<QString, Artist*> loadedArtists;
    QHash<QString, QList<FileInfo *> > filesWaitingForArtists;
    QHash<QString, Album*> loadedAlbums;
    QHash<QString, QList<FileInfo *> > filesWaitingForAlbums;
    QStringList trackPaths;
    QStringList nontrackPaths;

    QStringList directoryBlacklist;
    QStringList fileExtensionsBlacklist;

};

#endif // COLLECTIONSCANNER_H
