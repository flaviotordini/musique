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
    Tags() : track(0), year(0) { };
    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    int length;

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

class CollectionScanner : public QThread {

    Q_OBJECT

public:
    CollectionScanner();
    void setDirectory(QDir directory);
    void setIncremental(bool incremental) {
        this->incremental = incremental;
    }
    void run();
    void stop();
    void complete();

signals:
    void progress(int);
    void finished();
    void error(QString message);

private slots:
    void popFromQueue();
    void giveThisFileAnArtist(FileInfo *file);
    void processArtist(FileInfo *file);
    void gotArtistInfo();
    void giveThisFileAnAlbum(FileInfo *file);
    void processAlbum(FileInfo *file);
    void gotAlbumInfo();
    void processTrack(FileInfo *file);
    void gotTrackInfo();

private:
    void scanDirectory(QDir directory);
    void processFile(QFileInfo fileInfo);
    void cleanStaleTracks();

    bool working;
    bool incremental;
    QDir rootDirectory;
    uint lastUpdate;

    QList<QFileInfo> fileQueue;
    int maxQueueSize;
    QHash<QString, Artist*> loadedArtists;
    QHash<QString, QList<FileInfo *> > filesWaitingForArtists;
    QHash<QString, Album*> loadedAlbums;
    QHash<QString, QList<FileInfo *> > filesWaitingForAlbums;

};

#endif // COLLECTIONSCANNER_H
