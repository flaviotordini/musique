#include "collectionscannerthread.h"
#include "collectionscanner.h"

CollectionScannerThread::CollectionScannerThread(QObject *parent) : QThread(parent) {
    // This will be used by Database to cache connections for this thread
    setObjectName("scanner");
}

void CollectionScannerThread::run() {

    // qDebug() << "CollectionScannerThread::run()";

    scanner = new CollectionScanner(0);
    scanner->setDirectory(rootDirectory);
    connect(scanner, SIGNAL(progress(int)), SIGNAL(progress(int)), Qt::QueuedConnection);
    connect(scanner, SIGNAL(error(QString)), SIGNAL(error(QString)), Qt::QueuedConnection);
    connect(scanner, SIGNAL(finished()), SLOT(finish()), Qt::QueuedConnection);
    connect(scanner, SIGNAL(finished()), SIGNAL(finished()), Qt::QueuedConnection);
    scanner->run();

    // Start thread event loop
    // This makes signals and slots work
    exec();

    // qDebug() << "CollectionScannerThread::run() exited";

}

void CollectionScannerThread::setDirectory(QString directory) {
    rootDirectory = directory;
}

void CollectionScannerThread::cleanup() {
    // qDebug() << "Cleanup";
    delete scanner;
    exit();
}

void CollectionScannerThread::finish() {
    // qDebug() << "Finish";
    QTimer::singleShot(0, this, SLOT(cleanup()));
}
