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

#include "infoview.h"
#include "context/albuminfo.h"
#include "context/artistinfo.h"
#include "context/trackinfo.h"
#include "model/track.h"

InfoView::InfoView(QWidget *parent) : QWidget(parent) {
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    scrollArea = new QScrollArea();
    scrollArea->setPalette(parent->palette());
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollingInfoView = new ScrollingInfoView(this);
    scrollArea->setWidget(scrollingInfoView);
    layout->addWidget(scrollArea);
}

void InfoView::setTrack(Track *track) {
    setUpdatesEnabled(false);
    scrollingInfoView->artistInfo->setArtist(track->getArtist());
    scrollingInfoView->albumInfo->setAlbum(track->getAlbum());
    scrollingInfoView->trackInfo->setTrack(track);
    scrollingInfoView->adjustSize();
    scrollArea->ensureVisible(0, 0, 1, 1);
    setUpdatesEnabled(true);
}

ScrollingInfoView::ScrollingInfoView(QWidget *parent) : QWidget(parent) {
    QBoxLayout *layout = new QHBoxLayout(this);
    const int padding = 20;
    layout->setSpacing(padding);
    layout->setContentsMargins(padding, padding, padding, padding);

    artistInfo = new ArtistInfo(this);
    artistInfo->setMaximumWidth(300);
    layout->addWidget(artistInfo);

    trackInfo = new TrackInfo(this);
    layout->addWidget(trackInfo, 1, Qt::AlignHCenter);

    albumInfo = new AlbumInfo(this);
    albumInfo->setMaximumWidth(300);
    layout->addWidget(albumInfo);

    setLayout(layout);
}
