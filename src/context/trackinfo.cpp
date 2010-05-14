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

    /*
    trackNumberLabel = new QLabel(this);
    layout->addWidget(trackNumberLabel);
    */

    lyricsLabel = new QLabel(this);
    lyricsLabel->setTextFormat(Qt::RichText);
    lyricsLabel->setAlignment(Qt::AlignTop);
    lyricsLabel->setOpenExternalLinks(true);
    lyricsLabel->setWordWrap(true);
    lyricsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
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
    lyricsLabel->setText(lyrics);
}

void TrackInfo::clear() {
    titleLabel->clear();
    trackNumberLabel->clear();
    lyricsLabel->clear();
}
