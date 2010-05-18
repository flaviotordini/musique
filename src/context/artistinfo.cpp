#include "artistinfo.h"
#include "../model/artist.h"
#include "../fontutils.h"

ArtistInfo::ArtistInfo(QWidget *parent) :
        QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->setSpacing(20);
    layout->setMargin(20);

    titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    titleLabel->setFont(FontUtils::bigBold());
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(titleLabel);

    photoLabel = new QLabel(this);
    /*
    QGraphicsDropShadowEffect * effect = new QGraphicsDropShadowEffect();
    effect->setXOffset(0);
    effect->setYOffset(0);
    effect->setColor(QColor(64, 64, 64, 128));
    effect->setBlurRadius(20.0);
    photoLabel->setGraphicsEffect(effect);
    */
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

    QString htmlBio = "<html><style>a { color: white }</style><body>" + bio.left(split);
    if (split != -1) {
        QString bioUrl = "http://www.last.fm/music/" + artist->getName() + "/+wiki";
        htmlBio += QString(" <a href='%1'>%2</a>").arg(bioUrl, tr("Read more"));
    }
    htmlBio += "</body></html>";
    bioLabel->setText(htmlBio);

    photoLabel->setPixmap(QPixmap::fromImage(artist->getPhoto()));

}

void ArtistInfo::clear() {
    titleLabel->clear();
    photoLabel->clear();
    bioLabel->clear();
}
