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

#include "trackinfo.h"
#include "../model/track.h"
#include "fontutils.h"

TrackInfo::TrackInfo(QWidget *parent) : QWidget(parent) {
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    const int padding = 20;
    layout->setSpacing(padding);
    layout->setContentsMargins(0, 0, 0, 0);

    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignHCenter);
    titleLabel->setWordWrap(true);
    titleLabel->setFont(FontUtils::big());
    titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(titleLabel);

    /*
    trackNumberLabel = new QLabel(this);
    layout->addWidget(trackNumberLabel);
    */

    lyricsLabel = new QLabel(this);
    lyricsLabel->setTextFormat(Qt::RichText);
    lyricsLabel->setFont(FontUtils::medium());
    lyricsLabel->setAlignment(Qt::AlignHCenter);
    lyricsLabel->setWordWrap(true);
    lyricsLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    layout->addWidget(lyricsLabel);
}

void TrackInfo::setTrack(Track *track) {
    if (!track) {
        clear();
        return;
    }

    titleLabel->setText(track->getTitle());

    // trackNumberLabel->setText(QString::number(track->getNumber()));

    lyricsLabel->clear();
    connect(track, SIGNAL(gotLyrics(QString)), this, SLOT(showLyrics(QString)));
    track->getLyrics();

}

void TrackInfo::showLyrics(QString lyrics) {
    lyrics.replace('\n', "<br>");
    lyricsLabel->setText("<div style='line-height:130%'>" + lyrics + "</div>");
}

void TrackInfo::clear() {
    titleLabel->clear();
    // trackNumberLabel->clear();
    lyricsLabel->clear();
}
