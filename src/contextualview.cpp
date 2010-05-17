#include "contextualview.h"
#include "context/artistinfo.h"
#include "context/albuminfo.h"
#include "context/trackinfo.h"
#include "model/track.h"

ContextualView::ContextualView(QWidget *parent) :
        QScrollArea(parent) {
    scrollingContextualView = new ScrollingContextualView(this);
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setWidget(scrollingContextualView);
}

void ContextualView::setTrack(Track *track) {
    setUpdatesEnabled(false);
    scrollingContextualView->artistInfo->setArtist(track->getArtist());
    scrollingContextualView->albumInfo->setAlbum(track->getAlbum());
    scrollingContextualView->trackInfo->setTrack(track);
    scrollingContextualView->adjustSize();
    ensureVisible(0, 0, 1, 1);
    setUpdatesEnabled(true);
}

void ContextualView::disappear() {

}

ScrollingContextualView::ScrollingContextualView(QWidget *parent) :
        QWidget(parent) {

    // colors
    QPalette p = palette();
    p.setBrush(QPalette::Window, QColor(0x30, 0x30, 0x30));
    p.setBrush(QPalette::Foreground, QColor(0xdc, 0xdc, 0xdc));
    p.setBrush(QPalette::Base, Qt::red);
    p.setBrush(QPalette::Text, Qt::white);
    p.setColor(QPalette::Link, Qt::white);
    p.setBrush(QPalette::LinkVisited, Qt::white);
    this->setPalette(p);

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);

    artistInfo = new ArtistInfo(this);
    artistInfo->setPalette(p);
    layout->addWidget(artistInfo);

    albumInfo = new AlbumInfo(this);
    albumInfo->setPalette(p);
    layout->addWidget(albumInfo);

    trackInfo = new TrackInfo(this);
    trackInfo->setPalette(p);
    layout->addWidget(trackInfo);

    setLayout(layout);
    
}

void ScrollingContextualView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    const int gradientHeight = parentWidget()->height();
    QLinearGradient linearGrad(0, 0, 0, gradientHeight);
    linearGrad.setColorAt(0, QColor(0x0, 0x0, 0x0));
    linearGrad.setColorAt(1, QColor(0x30, 0x30, 0x30));
    painter.fillRect(0, 0, width(), gradientHeight, QBrush(linearGrad));
}
