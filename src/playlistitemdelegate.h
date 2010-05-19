#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QtGui>

class Track;

class PlaylistItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    PlaylistItemDelegate(QObject *parent = 0);
    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private:
    void paintTrack(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintAlbumHeader(QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const;
    void paintTrackNumber(QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const;
    void paintTrackTitle(QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const;
    void paintTrackLength(QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const;
    void paintActiveOverlay(QPainter *painter, const QStyleOptionViewItem& option, QRect line) const;

    static const int PADDING;
    static int ITEM_HEIGHT;

};

#endif // PLAYLISTITEMDELEGATE_H
