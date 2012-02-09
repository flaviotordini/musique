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
        AlbumType = 1
    };

    enum ImageDownloadStatuses {
        WaitingStatus = 0,
        ProcessingStatus = 1,
        FailedStatus = 2
    };

    static void enqueue(int objectId, int objectType, QString url);
    static void clearQueue();

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
