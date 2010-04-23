#include "artistinfo.h"
#include "../model/artist.h"
#include "../fontutils.h"

ArtistInfo::ArtistInfo(QWidget *parent) :
        QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->setSpacing(20);
    layout->setMargin(20);

    artistLabel = new QLabel(this);
    artistLabel->setWordWrap(true);
    artistLabel->setFont(FontUtils::bigBold());
    layout->addWidget(artistLabel);

    artistPhoto = new QLabel(this);
    layout->addWidget(artistPhoto);

    artistBio = new QLabel(this);
    artistBio->setAlignment(Qt::AlignTop);
    artistBio->setOpenExternalLinks(true);
    artistBio->setWordWrap(true);
    artistBio->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    layout->addWidget(artistBio);

    artistBioMore = new QLabel(this);
    artistBioMore->setAlignment(Qt::AlignTop);
    artistBioMore->setOpenExternalLinks(true);
    artistBioMore->setWordWrap(true);
    artistBioMore->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    artistBioMore->hide();
    layout->addWidget(artistBioMore);

}

void ArtistInfo::setArtist(Artist *artist) {
    if (!artist) return;

    artistLabel->setText(artist->getName());

    QString bio = artist->getBio();
    int bioSplit = bio.indexOf('\n');
    qDebug() << bio;
    artistBio->setText(
            "<html><style>a { color: white }</style><body>"
            + bio.left(bioSplit) + QString(" <a href='#readmore'>%1</a>").arg(tr("Read more"))
            + "</body></html>"
            );
    artistBioMore->setText(bio.right(bioSplit));
    artistPhoto->setPixmap(QPixmap::fromImage(artist->getPhoto()));

}
