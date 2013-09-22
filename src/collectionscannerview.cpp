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

#include "collectionscannerview.h"
#include "constants.h"
#include "fontutils.h"
#include "database.h"

static const int PADDING = 30;

CollectionScannerView::CollectionScannerView( QWidget *parent ) : QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(PADDING);
    layout->setMargin(PADDING);

    QLabel *tipLabel = new QLabel(
            tr("%1 is scanning your music collection.").arg(Constants::NAME)
            , this);
    tipLabel->setFont(FontUtils::bigger());
    layout->addWidget(tipLabel);

    progressBar = new QProgressBar(this);
    progressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(progressBar);

    tipLabel = new QLabel("<html><style>a { color: palette(text); }</style><body>" +
            tr("%1 is using <a href='%2'>%3</a> to catalog your music.")
            .arg(Constants::NAME, "http://last.fm", "Last.fm")
            + " " +
            tr("This will take time depending on your collection size and network speed.")
            + "</body></html>"
            , this);
    tipLabel->setOpenExternalLinks(true);
    tipLabel->setWordWrap(true);
    layout->addWidget(tipLabel);

}

void CollectionScannerView::setCollectionScannerThread(CollectionScannerThread *scannerThread) {

    // qDebug() << "CollectionScannerView::startScan" << directory;

    progressBar->setMaximum(0);

    connect(scannerThread, SIGNAL(progress(int)), SLOT(progress(int)), Qt::QueuedConnection);
    connect(scannerThread, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)), Qt::QueuedConnection);
    connect(scannerThread, SIGNAL(finished()), window(), SLOT(showMediaView()), Qt::QueuedConnection);
    connect(scannerThread, SIGNAL(finished()), SLOT(scanFinished()), Qt::QueuedConnection);

}

void CollectionScannerView::scanFinished() {
    window()->activateWindow();
}

void CollectionScannerView::scanError(QString message) {
    qDebug() << message;
}

void CollectionScannerView::progress(int value) {
    if (value > 0 && progressBar->maximum() != 100) progressBar->setMaximum(100);
}

void CollectionScannerView::paintEvent(QPaintEvent * /*event*/) {
#if defined(APP_MAC) | defined(APP_WIN)
    QBrush brush;
    if (window()->isActiveWindow()) {
        brush = QBrush(QColor(0xdd, 0xe4, 0xeb));
    } else {
        brush = palette().window();
    }
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), brush);
#endif
}
