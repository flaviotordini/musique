#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

#include <QtGui>

class Artist;
class Album;
class Folder;

class FinderItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    FinderItemDelegate( QObject* parent = 0 );
    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private:
    QPixmap createPlayIcon(bool hovered) const;
    QPixmap createMissingItemBackground() const;
    QPixmap createMissingItemPixmap(QString type) const;
    QPixmap getPlayIcon(bool hovered) const;
    QPixmap getMissingItemBackground() const;
    QPixmap getMissingArtistPixmap() const;
    QPixmap getMissingAlbumPixmap() const;
    QPixmap getMissingTrackPixmap() const;
    void paintArtist( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintAlbum( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintFolder( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintTrack( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintPlayIcon(QPainter *painter, const QRect& rect, double animation = 0., bool hoverAnimation = false) const;
    void drawName(QPainter *painter, const QStyleOptionViewItem &option, QString time, const QRect&, bool selected) const;
    void drawBadge(QPainter *painter, QString text, const QRect&) const;
    void drawCentralLabel(QPainter *painter, QString text, const QRect&) const;
    QPixmap getArtistPixmap(Artist*) const;
    QPixmap getFolderPixmap(Folder*) const;

    static const int ITEM_WIDTH;
    static const int ITEM_HEIGHT;
    static const int PADDING;

};

#endif // ITEMDELEGATE_H
