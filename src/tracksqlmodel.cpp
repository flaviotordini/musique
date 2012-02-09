#include "tracksqlmodel.h"

TrackSqlModel::TrackSqlModel(QObject *parent) : BaseSqlModel(parent) {

}

QVariant TrackSqlModel::data(const QModelIndex &index, int role) const {

    Track *track = 0;
    int trackId = 0;

    switch (role) {

    case Qt::DisplayRole:
        trackId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        track = Track::forId(trackId);
        return track->getTitle();

    case Finder::ItemTypeRole:
        return Finder::ItemTypeTrack;

    case Finder::DataObjectRole:
        trackId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        track = Track::forId(trackId);
        return QVariant::fromValue(QPointer<Track>(track));

    case Finder::HoveredItemRole:
        return hoveredRow == index.row();

    case Finder::PlayIconAnimationItemRole:
        return timeLine->currentFrame() / 1000.;

    case Finder::PlayIconHoveredRole:
        return playIconHovered;

    case Qt::StatusTipRole:
        trackId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        track = Track::forId(trackId);
        return track->getStatusTip();

    }

    return QVariant();
}

