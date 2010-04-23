#include "artistsqlmodel.h"

ArtistSqlModel::ArtistSqlModel(QObject *parent) : BaseSqlModel(parent) {

}

QVariant ArtistSqlModel::data(const QModelIndex &index, int role) const {

    Artist *artist = 0;
    int artistId = 0;

    switch (role) {

    case Finder::ItemTypeRole:
        return Finder::ItemTypeArtist;

    case Finder::DataObjectRole:
        artistId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        artist = Artist::forId(artistId);
        return QVariant::fromValue(QPointer<Artist>(artist));

    case Finder::HoveredItemRole:
        return hoveredRow == index.row();

    case Finder::PlayIconAnimationItemRole:
        return timeLine->currentFrame() / 1000.;

    case Finder::PlayIconHoveredRole:
        return playIconHovered;

    }

    return QVariant();
}

