#include "contextualview.h"
#include "context/artistinfo.h"
#include "context/albuminfo.h"
#include "context/trackinfo.h"
#include "model/track.h"

ContextualView::ContextualView(QWidget *parent) :
        QScrollArea(parent) {
    setPalette(parent->palette());
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

void ScrollingContextualView::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);

    const int hCenter = width() * .5;
    QRadialGradient gradient(hCenter, 0,
                             width(),
                             hCenter, 0);
    gradient.setColorAt(1, QColor(0x00, 0x00, 0x00));
    gradient.setColorAt(0, QColor(0x4c, 0x4c, 0x4c));

    QRect rect = visibleRegion().boundingRect();
    painter.fillRect(rect, QBrush(gradient));

    QLinearGradient shadow;
    shadow.setFinalStop(0, 20);
    shadow.setColorAt(0, QColor(0x00, 0x00, 0x00, 48));
    shadow.setColorAt(1, QColor(0x00, 0x00, 0x00, 0));
    painter.fillRect(rect.x(), rect.y(), rect.width(), 20, QBrush(shadow));

}
