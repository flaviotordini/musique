#include "scanningview.h"
#include "collectionscannerthread.h"

ScanningView::ScanningView( QWidget *parent ) : QWidget(parent) {

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);

    progressBar = new QProgressBar(this);
    layout->addWidget(progressBar);

    CollectionScannerThread *scanner = new CollectionScannerThread();
    connect(scanner, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
    connect(scanner, SIGNAL(finished()), parent, SLOT(showMediaView()));
    // scanner->setDirectory(QDir("/home/flavio/Music/italiana"));
    scanner->setDirectory(QDir("/Users/flavio/Music"));
    scanner->start(); // QThread::IdlePriority

}
