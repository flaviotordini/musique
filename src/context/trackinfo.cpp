#include "trackinfo.h"
#include "../fontutils.h"
#include "../model/track.h"

TrackInfo::TrackInfo(QWidget *parent) :
        QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->setSpacing(20);
    layout->setMargin(20);

    titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    titleLabel->setFont(FontUtils::bigBold());
    layout->addWidget(titleLabel);

    trackNumberLabel = new QLabel(this);
    layout->addWidget(trackNumberLabel);

    lyricsLabel = new QLabel(this);
    lyricsLabel->setAlignment(Qt::AlignTop);
    lyricsLabel->setOpenExternalLinks(true);
    lyricsLabel->setWordWrap(true);
    lyricsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(lyricsLabel);

}

void TrackInfo::setTrack(Track *track) {
    if (!track) {
        clear();
        return;
    }

    titleLabel->setText(track->getTitle());

    trackNumberLabel->setText(QString::number(track->getNumber()));

    QString lyrics = track->getLyrics();
    lyricsLabel->setText(lyrics);

}

void TrackInfo::clear() {
    titleLabel->clear();
    trackNumberLabel->clear();
    lyricsLabel->clear();
}
