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

#include "artistsqlmodel.h"
#include "mainwindow.h"

ArtistSqlModel::ArtistSqlModel(QObject *parent) : BaseSqlModel(parent) {

}

QVariant ArtistSqlModel::data(const QModelIndex &index, int role) const {

    Artist *artist = nullptr;
    int artistId = 0;

    switch (role) {

    case Finder::ItemTypeRole:
        return Finder::ItemTypeArtist;

    case Finder::DataObjectRole:
        artistId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        artist = Artist::forId(artistId);
        connect(artist, SIGNAL(gotPhoto()), MainWindow::instance(), SLOT(update()), Qt::UniqueConnection);
        return QVariant::fromValue(QPointer<Artist>(artist));

    case Finder::HoveredItemRole:
        return hoveredRow == index.row();

    case Finder::PlayIconAnimationItemRole:
        return timeLine->currentFrame() / 1000.;

    case Finder::PlayIconHoveredRole:
        return playIconHovered;

    case Qt::StatusTipRole:
        artistId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        artist = Artist::forId(artistId);
        return artist->getStatusTip();

    }

    return QVariant();
}

