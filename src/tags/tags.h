#ifndef TAGS_H
#define TAGS_H

#include <QtCore>

class Tags : public QObject {

    Q_OBJECT

public:
    Tags() :
        duration(0),
        compilation(false),
        trackNumber(0),
        trackCount(0),
        diskNumber(1),
        diskCount(1),
        year(0)
      // bpm(0)
    { }

    const QString &getFilename() const { return filename; }
    void setFilename(const QString &value) { filename = value; }

    int getDuration() const { return duration; }
    void setDuration(int value) { duration = value; }

    const QString &getTitle() const { return title; }
    void setTitle(const QString &value) { title = value; }

    QString getAlbumString() const { return albumString; }
    void setAlbumString(const QString &value) { albumString = value; }

    QString getArtistString() const { return artistString; }
    void setArtistString(const QString &value) { artistString = value; }

    QString getArtistSort() const { return artistSort; }
    void setArtistSort(const QString &value) { artistSort = value; }

    QString getAlbumArtist() const { return albumArtist; }
    void setAlbumArtist(const QString &value) { albumArtist = value; }

    QString getAlbumArtistSort() const { return albumArtistSort; }
    void setAlbumArtistSort(const QString &value) { albumArtistSort = value; }

    bool isCompilation() const { return compilation; }
    void setCompilation(bool value) { compilation = value; }

    QString getComposer() const { return composer; }
    void setComposer(const QString &value) { composer = value; }

    QString getComposerSort() const { return composerSort; }
    void setComposerSort(const QString &value) { composerSort = value; }

    QString getGenre() const { return genre; }
    void setGenre(const QString &value) { genre = value; }

    int getTrackNumber() const { return trackNumber; }
    void setTrackNumber(int value) { trackNumber = value; }

    int getTrackCount() const { return trackCount; }
    void setTrackCount(int value) { trackCount = value; }

    int getDiskNumber() const { return diskNumber; }
    void setDiskNumber(int value) { diskNumber = value; }

    int getDiskCount() const { return diskCount; }
    void setDiskCount(int value) { diskCount = value; }

    int getYear() const { return year; }
    void setYear(int value) { year = value; }

    QString getLyrics() const { return lyrics; }
    void setLyrics (const QString &value) { lyrics = value; }

    QString getComment() const { return comment; }
    void setComment (const QString &value) { comment = value; }

private:
    QString filename;
    int duration;
    QString title;
    QString albumString;
    QString artistString;
    QString artistSort;
    QString albumArtist;
    QString albumArtistSort;
    bool compilation;
    QString composer;
    QString composerSort;
    QString genre;
    int trackNumber;
    int trackCount;
    int diskNumber;
    int diskCount;
    int year;
    QString lyrics;
    QString comment;
    // TODO int bpm;

};

#endif // TAGS_H
