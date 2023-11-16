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

#include "albuminfo.h"
#include "../database.h"
#include "../model/album.h"
#include "../tracklistview.h"
#include "../tracksqlmodel.h"
#include "fontutils.h"
#include "painterutils.h"

AlbumInfo::AlbumInfo(QWidget *parent) :
        QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    const int padding = 20;
    layout->setSpacing(padding);
    layout->setContentsMargins(0, 0, 0, 0);

    titleLabel = new QLabel(this);
    titleLabel->setWordWrap(true);
    titleLabel->setFont(FontUtils::big());
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(titleLabel);

    photoLabel = new QLabel(this);
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

        QString html = "<html><style>"
                       "body { line-height:130% }"
                       "a { color:palette(window-text);font-weight:bold;text-decoration:none }"
                       "</style><body>" +
                       wiki.left(split);
        if (split != -1) {
            Artist *artist = album->getArtist();
            QString url = "http://www.last.fm/music/" + (artist ? artist->getName() : "_") + "/" + album->getTitle() + "/+wiki";
            html += QString(" <a href='%1'>%2</a>").arg(url, tr("Read more"));
        }
        html += "</body></html>";
        wikiLabel->setText(html);
    }

    QPixmap p(album->getImageLocation());
    if (p.isNull()) {
        photoLabel->clear();
        photoLabel->hide();
    } else {
        p.setDevicePixelRatio(devicePixelRatio());
        int maxWidth = 300 * devicePixelRatio();
        qDebug() << p.width();
        if (p.width() != maxWidth) p = p.scaledToWidth(maxWidth, Qt::SmoothTransformation);
        photoLabel->setPixmap(PainterUtils::roundCorners(p));
        photoLabel->show();
    }

#ifdef APP_AFFILIATE_AMAZON
    QString query;
    if (album->getArtist())
        query = album->getArtist()->getName() + " - ";
    query += album->getTitle();
    buyOnAmazonButton->setProperty("query", query);
    buyOnAmazonButton->show();
#endif

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
#ifdef APP_AFFILIATE_AMAZON
    buyOnAmazonButton->hide();
    buyOnAmazonButton->setProperty("query", QVariant());
#endif
    // trackListModel->clear();
}

#ifdef APP_AFFILIATE_AMAZON
void AlbumInfo::amazonClicked() {
    QString query = buyOnAmazonButton->property("query").toString();

    // http://www.amazon.com/gp/search?ie=UTF8&keywords=Metallica+-+Master-of-puppets&tag=flavtord-20&index=music&linkCode=ur2&camp=1789&creative=9325
    QUrl url("http://www.amazon.com/gp/search");
    QUrlQuery q;
    q.addQueryItem("ie", "UTF8");
    q.addQueryItem("keywords", query);
    q.addQueryItem("tag", "flavtord-20");
    q.addQueryItem("index", "music");
    q.addQueryItem("linkCode", "ur2");
    q.addQueryItem("camp", "1789");
    q.addQueryItem("creative", "9325");
    url.setQuery(q);
    QDesktopServices::openUrl(url);
}
#endif
