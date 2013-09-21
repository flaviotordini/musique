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

    enum ImageDownloadTypes {
        ArtistType = 0,
        AlbumType
    };

    static void enqueue(int objectId, int objectType, QString url);

public slots:
    void imageDownloaded(QByteArray bytes);
    void imageDownloadError(QNetworkReply *reply);

private:
    void popFromQueue();
    ImageDownload* imageDownload;

};

class ImageDownloaderThread : public QThread {

    Q_OBJECT

public:
    ImageDownloaderThread(QObject *parent = 0);
    void run();

signals:
    void progress(int);
    void error(QString message);

private slots:

private:
    ImageDownloader* imageDownloader;

};

#endif // IMAGEDOWNLOADER_H
