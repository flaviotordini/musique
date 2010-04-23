#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QtGui>

class PlaylistItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    PlaylistItemDelegate(QObject *parent = 0);
    // QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private:
    void paintTrack( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    static const int PADDING;

};

#endif // PLAYLISTITEMDELEGATE_H
