#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QtWidgets>

class Track;
class PlaylistView;

class PlaylistItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    PlaylistItemDelegate(PlaylistView *parent);
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;

private:
    void paintTrack(QPainter *painter,
                    const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void paintAlbumHeader(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QRect &line,
                          Track *track) const;
    void paintTrackNumber(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QRect &line,
                          Track *track) const;
    void paintTrackTitle(QPainter *painter,
                         const QStyleOptionViewItem &option,
                         const QRect &line,
                         Track *track) const;
    void paintTrackLength(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QRect &line,
                          Track *track) const;

    PlaylistView *view;
};

#endif // PLAYLISTITEMDELEGATE_H
