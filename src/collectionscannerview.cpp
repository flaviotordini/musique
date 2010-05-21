#include "collectionscannerview.h"
#include "constants.h"
#include "fontutils.h"
#include "database.h"

static const int PADDING = 30;

CollectionScannerView::CollectionScannerView( QWidget *parent ) : QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(PADDING);
    layout->setMargin(0);

    QLabel *tipLabel = new QLabel(
            tr("%1 is scanning your music collection.").arg(Constants::APP_NAME)
            , this);
    tipLabel->setFont(FontUtils::big());
    layout->addWidget(tipLabel);

    progressBar = new QProgressBar(this);
    progressBar->setMaximum(0);
    progressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(progressBar);

    tipLabel = new QLabel(
            tr("%1 is using <a href='%2'>%3</a> to catalog your music.")
            .arg(Constants::APP_NAME, "http://last.fm", "Last.fm")
            + " " +
            tr("This will take time depending on your collection size and network speed.")

            , this);
    tipLabel->setOpenExternalLinks(true);
    tipLabel->setWordWrap(true);
    layout->addWidget(tipLabel);

}

void CollectionScannerView::startScan(QString dir) {
    /*
    if (scanner) {
        qDebug("Hey CollectionScannerThread was not deleted...");
        delete scanner;
    }
    */

    qDebug() << "CollectionScannerView::startScan" << dir;

    progressBar->setMaximum(0);

    CollectionScannerThread &scanner = CollectionScannerThread::instance();
    connect(&scanner, SIGNAL(progress(int)), this, SLOT(progress(int)));
    connect(&scanner, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
    connect(&scanner, SIGNAL(finished()), window(), SLOT(showMediaView()));
    connect(&scanner, SIGNAL(finished()), SLOT(scanFinished()));
    scanner.setDirectory(QDir(dir));

    Database &db = Database::instance();
    if (db.status() == ScanComplete) {
        scanner.setIncremental(false);
    } else {
        scanner.setIncremental(true);
    }

    qDebug() << "CollectionScannerView:: scanner.start()";
    scanner.start();
    qDebug() << "CollectionScannerView::scanner.start() ended";
}

void CollectionScannerView::scanFinished() {
    // if (scanner) delete scanner;
    window()->activateWindow();
}

void CollectionScannerView::scanError(QString message) {
    qDebug() << message;
}

void CollectionScannerView::progress(int value) {
    if (value > 0 && progressBar->maximum() != 100) progressBar->setMaximum(100);
}
