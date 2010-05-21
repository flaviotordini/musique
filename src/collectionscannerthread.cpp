#include "collectionscannerthread.h"
#include "collectionscanner.h"

static CollectionScannerThread *threadInstance = 0;

CollectionScannerThread::CollectionScannerThread() {
    // This will be used by Database to cache connections for this thread
    setObjectName("scanner");
}

CollectionScannerThread& CollectionScannerThread::instance() {
    /*
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!threadInstance) threadInstance = new CollectionScannerThread();
    return *threadInstance;
    */
    CollectionScannerThread *cst = new CollectionScannerThread();
    return *cst;
}

void CollectionScannerThread::run() {

    qDebug() << "CollectionScannerThread::run()";

    CollectionScanner *scanner = new CollectionScanner();
    scanner->setDirectory(rootDirectory);
    scanner->setIncremental(incremental);
    connect(scanner, SIGNAL(progress(int)), SIGNAL(progress(int)), Qt::QueuedConnection);
    connect(scanner, SIGNAL(error(QString)), SIGNAL(error(QString)));
    connect(scanner, SIGNAL(finished()), SIGNAL(finished()));
    scanner->run();

    // Start thread event loop
    // This makes signals and slots work
    exec();

    delete scanner;

    qDebug() << "CollectionScannerThread::run() exited";

}

void CollectionScannerThread::setDirectory(QDir directory) {
    rootDirectory = directory;
}

void CollectionScannerThread::setIncremental(bool incremental) {
    this->incremental = incremental;
}
