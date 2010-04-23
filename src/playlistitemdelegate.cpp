#include "playlistitemdelegate.h"
#include "model/track.h"
#include "playlistmodel.h"

PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent) :
        QStyledItemDelegate(parent) {

}

void PlaylistItemDelegate::paint( QPainter* painter,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index ) const {

    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );
    paintTrack(painter, option, index);

}

void PlaylistItemDelegate::paintTrack(QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const {

    // get the data object
    const TrackPointer trackPointer = index.data(Playlist::DataObjectRole).value<TrackPointer>();
    Track *track = trackPointer.data();

}
