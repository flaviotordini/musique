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
    int getTrackCount();
    int getTotalLength();
    
private:
    Album* getAlbum();
    Artist* getArtist();

    QDir dir;
    QString path;
    int trackCount;
    int totalLength;

};

// This is required in order to use QPointer<Folder> as a QVariant
typedef QPointer<Folder> FolderPointer;
Q_DECLARE_METATYPE(FolderPointer);

#endif // FOLDER_H
