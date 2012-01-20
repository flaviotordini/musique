#include "albumsqlmodel.h"
#include "model/album.h"

AlbumSqlModel::AlbumSqlModel(QObject *parent) : BaseSqlModel(parent) {
}

QVariant AlbumSqlModel::data(const QModelIndex &index, int role) const
{

    Album* album = 0;

    switch (role) {

    case Finder::ItemTypeRole:
        return Finder::ItemTypeAlbum;
        break;

    case Finder::DataObjectRole:
        album = Album::forId(QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt());
        return QVariant::fromValue(QPointer<Album>(album));
        break;

    case Finder::HoveredItemRole:
        return hoveredRow == index.row();
        break;

    case Finder::PlayIconAnimationItemRole:
        return timeLine->currentFrame() / 1000.;

    case Finder::PlayIconHoveredRole:
        return playIconHovered;

    case Qt::StatusTipRole:
        album = Album::forId(QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt());
        return album->getStatusTip();

    }

    return QVariant();
}
