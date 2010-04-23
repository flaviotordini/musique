#include "albuminfo.h"
#include "../model/album.h"
#include "../fontutils.h"
#include "../tracklistview.h"
#include "../tracksqlmodel.h"
#include "../database.h"

AlbumInfo::AlbumInfo(QWidget *parent) :
        QWidget(parent)
{

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->setSpacing(20);
    layout->setMargin(20);

    titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    titleLabel->setFont(FontUtils::bigBold());
    layout->addWidget(titleLabel);

    photoLabel = new QLabel(this);
    layout->addWidget(photoLabel);

    wikiLabel = new QLabel(this);
    wikiLabel->setAlignment(Qt::AlignTop);
    wikiLabel->setOpenExternalLinks(true);
    wikiLabel->setWordWrap(true);
    wikiLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    layout->addWidget(wikiLabel);

    wikiMoreLabel = new QLabel(this);
    wikiMoreLabel->setAlignment(Qt::AlignTop);
    wikiMoreLabel->setOpenExternalLinks(true);
    wikiMoreLabel->setWordWrap(true);
    wikiMoreLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    wikiMoreLabel->hide();
    layout->addWidget(wikiMoreLabel);

    trackListView = new TrackListView(this);
    trackListView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    trackListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    trackListModel = new TrackSqlModel(this);
    trackListView->setModel(trackListModel);
    layout->addWidget(trackListView);

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
        wikiMoreLabel->clear();
    } else {
        int wikiSplit = wiki.indexOf('\n');
        qDebug() << wiki;
        wikiLabel->setText(
                "<html><style>a { color: white }</style><body>"
                + wiki.left(wikiSplit) + QString(" <a href='#readmore'>%1</a>").arg(tr("Read more"))
                + "</body></html>"
                );
        wikiMoreLabel->setText(wiki.right(wikiSplit));
    }
    photoLabel->setPixmap(QPixmap::fromImage(album->getPhoto()));

    QString qry("SELECT id FROM tracks where album=%1 order by track, title");
    qry = qry.arg(album->getId());
    qDebug() << qry;
    trackListModel->setQuery(qry, Database::instance().getConnection());
    if (trackListModel->lastError().isValid())
        qDebug() << trackListModel->lastError();
    trackListView->setMinimumHeight(trackListView->maximumViewportSize().height());

}

void AlbumInfo::clear() {
    titleLabel->clear();
    photoLabel->clear();
    wikiLabel->clear();
    wikiMoreLabel->clear();
    trackListModel->clear();
}
