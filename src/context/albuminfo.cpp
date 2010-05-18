#include "albuminfo.h"
#include "../model/album.h"
#include "../fontutils.h"
#include "../tracklistview.h"
#include "../tracksqlmodel.h"
#include "../database.h"

AlbumInfo::AlbumInfo(QWidget *parent) :
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

    wikiLabel = new QLabel(this);
    wikiLabel->setAlignment(Qt::AlignTop);
    wikiLabel->setOpenExternalLinks(true);
    wikiLabel->setWordWrap(true);
    wikiLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    layout->addWidget(wikiLabel);

    /*
    trackListView = new TrackListView(this);
    trackListView->setStyleSheet("background: transparent");
    trackListView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    trackListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    trackListModel = new TrackSqlModel(this);
    trackListView->setModel(trackListModel);
    // no user interaction
    // trackListView->setEnabled(false);
    trackListView->setDragEnabled(false);
    layout->addWidget(trackListView);
    */

}

void AlbumInfo::setAlbum(Album *album) {
    if (!album) {
        clear();
        return;
    }

    titleLabel->setText(album->getTitle());

    QString wiki = album->getWiki();
    if (wiki.isEmpty()) {
        wikiLabel->clear();
    } else {
        int split = wiki.indexOf('\n', 512);
        if (split == -1) {
            split = wiki.indexOf(". ", 512);
        }

        QString html = "<html><style>a { color: white }</style><body>" + wiki.left(split);
        if (split != -1) {
            Artist *artist = album->getArtist();
            QString url = "http://www.last.fm/music/" + (artist ? artist->getName() : "_") + "/" + album->getTitle() + "/+wiki";
            html += QString(" <a href='%1'>%2</a>").arg(url, tr("Read more"));
        }
        html += "</body></html>";
        wikiLabel->setText(html);
    }

    QImage photo = album->getPhoto();
    if (photo.isNull()) {
        photoLabel->clear();
        photoLabel->hide();
    } else {
        photoLabel->setPixmap(QPixmap::fromImage(photo));
        photoLabel->show();
    }

    /*
    QString qry("SELECT id FROM tracks where album=%1 order by track, title");
    qry = qry.arg(album->getId());
    qDebug() << qry;
    trackListModel->setQuery(qry, Database::instance().getConnection());
    if (trackListModel->lastError().isValid())
        qDebug() << trackListModel->lastError();
    trackListView->setMinimumHeight(trackListView->maximumViewportSize().height());
    */

}

void AlbumInfo::clear() {
    titleLabel->clear();
    photoLabel->clear();
    photoLabel->hide();
    wikiLabel->clear();
    // trackListModel->clear();
}
