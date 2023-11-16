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

#include "artistinfo.h"
#include "../model/artist.h"
#include "fontutils.h"
#include "painterutils.h"

ArtistInfo::ArtistInfo(QWidget *parent) : QWidget(parent) {
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->setSpacing(20);
    layout->setContentsMargins(0, 0, 0, 0);

    titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    titleLabel->setFont(FontUtils::big());
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(titleLabel);

    photoLabel = new QLabel(this);
    photoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(photoLabel);

    bioLabel = new QLabel(this);
    bioLabel->setTextFormat(Qt::RichText);
    bioLabel->setAlignment(Qt::AlignTop);
    bioLabel->setOpenExternalLinks(true);
    bioLabel->setWordWrap(true);
    bioLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    layout->addWidget(bioLabel);
}

void ArtistInfo::setArtist(Artist *artist) {
    if (!artist) {
        clear();
        return;
    }

    titleLabel->setText(artist->getName());

    QString bio = artist->getBio();
    int split = bio.indexOf('\n', 512);
    if (split == -1) {
        split = bio.indexOf(". ", 512);
    }

    QString htmlBio = "<html><style>"
                      "body { line-height:130% }"
                      "a { color:palette(window-text);font-weight:bold;text-decoration:none }"
                      "</style><body>" +
                      bio.left(split);
    if (split != -1) {
        QString bioUrl = "http://www.last.fm/music/" + artist->getName() + "/+wiki";
        htmlBio += QString(" <a href='%1'>%2</a>").arg(bioUrl, tr("Read more"));
    }
    htmlBio += "</body></html>";
    bioLabel->setText(htmlBio);

    QPixmap p(artist->getImageLocation());
    if (p.isNull()) {
        photoLabel->clear();
        photoLabel->hide();
    } else {
        p.setDevicePixelRatio(devicePixelRatio());
        int maxWidth = 300 * devicePixelRatio();
        if (p.width() != maxWidth) p = p.scaledToWidth(maxWidth, Qt::SmoothTransformation);
        photoLabel->setPixmap(PainterUtils::roundCorners(p));
        photoLabel->show();
    }
}

void ArtistInfo::clear() {
    titleLabel->clear();
    photoLabel->clear();
    bioLabel->clear();
}
