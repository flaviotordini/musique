#ifndef FOLDER_H
#define FOLDER_H

#include <QtCore>
#include <QImage>
#include "item.h"
#include "track.h"

class Album;
class Artist;

class Folder : public Item {

    Q_OBJECT

public:
    Folder(QString path, QObject *parent = 0);
    static Folder* forPath(QString path);
    QImage getPhoto();

    // item
    QList<Track*> getTracks();

    // properties
    QString getName()  { return dir.dirName(); }
    QString getPath() { return path; }
    QString getAbsolutePath() { return dir.absolutePath(); }
    
private:
    Album* getAlbum();
    Artist* getArtist();

    QDir dir;

    /**
      * This path is relative to the collection root
      */
    QString path;
    bool photoLoaded;
    QImage photo;

};

// This is required in order to use QPointer<Artist> as a QVariant
typedef QPointer<Folder> FolderPointer;
Q_DECLARE_METATYPE(FolderPointer);

#endif // FOLDER_H
