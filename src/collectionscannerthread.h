#ifndef COLLECTIONSCANNERTHREAD_H
#define COLLECTIONSCANNERTHREAD_H

#include <QtCore>

class CollectionScannerThread : public QThread {

    Q_OBJECT

public:
    static CollectionScannerThread& instance();
    void setDirectory(QDir directory);
    void setIncremental(bool incremental);
    void run();

signals:
    void progress(int);
    void finished();
    void error(QString message);

private:
    CollectionScannerThread();
    QDir rootDirectory;
    bool incremental;

};

#endif // COLLECTIONSCANNERTHREAD_H
