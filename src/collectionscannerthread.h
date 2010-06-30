#ifndef COLLECTIONSCANNERTHREAD_H
#define COLLECTIONSCANNERTHREAD_H

#include <QtCore>
class CollectionScanner;

class CollectionScannerThread : public QThread {

    Q_OBJECT

public:
    CollectionScannerThread(QObject *parent = 0);
    void setDirectory(QString directory);
    void run();

signals:
    void progress(int);
    void error(QString message);

private slots:
    void finish();
    void cleanup();

private:
    QString rootDirectory;
    CollectionScanner* scanner;

};

#endif // COLLECTIONSCANNERTHREAD_H
