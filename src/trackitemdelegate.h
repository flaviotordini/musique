#ifndef TRACKITEMDELEGATE_H
#define TRACKITEMDELEGATE_H

#include <QtGui>

class TrackItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    TrackItemDelegate(QObject *parent = 0);
    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private:
    void paintTrack( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

    static const int PADDING;

};

#endif // TRACKITEMDELEGATE_H
