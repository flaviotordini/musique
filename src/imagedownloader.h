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

#ifndef IMAGEDOWNLOADER_H
#define IMAGEDOWNLOADER_H

#include <QtCore>
#include <QtNetwork>

class ImageDownload;

class ImageDownloader : public QObject {
    Q_OBJECT

public:
    ImageDownloader(QObject *parent = 0);
    void run();

    enum ImageDownloadTypes { ArtistType = 0, AlbumType };

    static void enqueue(int objectId, int objectType, const QString &url);

public slots:
    void onImageDownloaded(const QByteArray &bytes);
    void imageDownloadError();

signals:
    void imageDownloaded();

private:
    void popFromQueue();
    ImageDownload *imageDownload;
};

class ImageDownloaderThread : public QThread {
    Q_OBJECT

public:
    ImageDownloaderThread(QObject *parent = 0);
    void run();

signals:
    void progress(int);
    void error(QString message);
    void imageDownloaded();

private slots:

private:
    ImageDownloader *imageDownloader;
};

#endif // IMAGEDOWNLOADER_H
