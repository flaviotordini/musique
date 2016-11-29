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
#include "iconutils.h"
#include "database.h"

CollectionScannerView::CollectionScannerView( QWidget *parent ) : View(parent) {

    const int padding = 30;

    connect(window()->windowHandle(), SIGNAL(screenChanged(QScreen*)), SLOT(screenChanged()));

    QBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->setSpacing(padding);
    vLayout->setMargin(padding);

    QBoxLayout *hLayout = new QHBoxLayout();
    vLayout->addLayout(hLayout);
    hLayout->setAlignment(Qt::AlignCenter);
    hLayout->setMargin(padding);
    hLayout->setSpacing(padding);

    logo = new QLabel();
    logo->setPixmap(IconUtils::pixmap(":/images/app.png"));
    hLayout->addWidget(logo, 0, Qt::AlignTop);

    QBoxLayout *layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(padding);
    hLayout->addLayout(layout);

    QLabel *tipLabel = new QLabel(
            tr("%1 is scanning your music collection.").arg(Constants::NAME)
            , this);
    tipLabel->setFont(FontUtils::big());
    layout->addWidget(tipLabel);

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    layout->addWidget(progressBar);

    tipLabel = new QLabel("<html><style>a { color: palette(text); }</style><body>" +
            // tr("%1 is using <a href='%2'>%3</a> to catalog your music.")
            // .arg(Constants::NAME, "http://last.fm", "Last.fm")
            // + " " +
            tr("This will take time depending on your collection size and network speed.")
            + "</body></html>"
            , this);
    tipLabel->setOpenExternalLinks(true);
    tipLabel->setWordWrap(true);
    layout->addWidget(tipLabel);

}

void CollectionScannerView::setCollectionScannerThread(CollectionScannerThread *scannerThread) {

    // qDebug() << "CollectionScannerView::startScan" << directory;

    progressBar->setMaximum(1);

    connect(scannerThread, SIGNAL(progress(int)), SLOT(progress(int)), Qt::QueuedConnection);
    connect(scannerThread, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)), Qt::QueuedConnection);
}

void CollectionScannerView::scanError(QString message) {
    qWarning() << message;
}

void CollectionScannerView::progress(int value) {
    if (value > 0 && progressBar->maximum() != 100) progressBar->setMaximum(100);
}

void CollectionScannerView::screenChanged() {
    logo->setPixmap(IconUtils::pixmap(":/images/app.png"));
}

void CollectionScannerView::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    QPainter painter(this);
    QBrush brush = window()->isActiveWindow() ? palette().base() : palette().window();
    painter.fillRect(rect(), brush);
}
