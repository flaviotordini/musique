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

void ScrollingContextualView::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);

    QPainter painter(this);
    QRect rect = visibleRegion().boundingRect();

    QLinearGradient shadow;
    shadow.setFinalStop(0, 20);
    shadow.setColorAt(0, QColor(0x00, 0x00, 0x00, 48));
    shadow.setColorAt(1, QColor(0x00, 0x00, 0x00, 0));
    painter.fillRect(rect.x(), rect.y(), rect.width(), 20, QBrush(shadow));
}
