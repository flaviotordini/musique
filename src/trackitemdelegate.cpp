#include "trackitemdelegate.h"
#include "finderwidget.h"

const int TrackItemDelegate::PADDING = 10;

TrackItemDelegate::TrackItemDelegate(QObject *parent) :
        QStyledItemDelegate(parent) {

}

QSize TrackItemDelegate::sizeHint( const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/ ) const {
    return QSize(30, 30);
}

void TrackItemDelegate::paint( QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index ) const {

    /*
    int itemType = index.data(Finder::ItemTypeRole).toInt();

    if (itemType == Finder::ItemTypeTrack) { */
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &option, painter );
    paintTrack(painter, option, index);
    /*
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    } */

}

void TrackItemDelegate::paintTrack(QPainter* painter,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {

    painter->save();

    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    const QPalette palette;

    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;
    const bool isHovered = index.data(Finder::HoveredItemRole).toBool();

    // draw the "current track" highlight underneath the text
    /*
    if (isActive && !isSelected) {
        paintActiveOverlay(painter, line.x(), line.y(), line.width(), line.height());
    }
    */

    // get the data object
    const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
    Track *track = trackPointer.data();

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = index.data(Finder::PlayIconAnimationItemRole).toDouble();
        bool playIconHovered = index.data(Finder::PlayIconHoveredRole).toBool();
    }

    QPointF textLoc(PADDING, 0);

    // track number
    painter->save();
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    QString trackString = QString("%1").arg(track->getNumber(), 2, 10, QChar('0'));
    QSizeF trackStringSize(QFontMetrics(painter->font()).size( Qt::TextSingleLine, trackString));
    QRect trackTextBox(textLoc.x(), textLoc.y(), trackStringSize.width(), line.height());
    QRect trackRoundedRect = trackTextBox;

    trackRoundedRect.setY((line.height() - trackStringSize.height()) / 2);
    trackRoundedRect.setHeight(trackStringSize.height());
    trackRoundedRect.adjust(-PADDING/2, -PADDING/3, PADDING/2, PADDING/3);

    painter->setOpacity(.75);
    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setBrush(Qt::white);
    painter->setPen(Qt::black);
    painter->drawRoundedRect(trackRoundedRect, PADDING/2, PADDING/2, Qt::AbsoluteSize);
    painter->drawText(trackTextBox, Qt::AlignCenter, trackString);
    painter->restore();

    // title
    QString titleString = track->getTitle();
    QSizeF titleStringSize(QFontMetrics(painter->font()).size( Qt::TextSingleLine, titleString));
    textLoc.setX(textLoc.x() + trackStringSize.width() + PADDING);
    QRect titleTextBox(textLoc.x(), textLoc.y(), titleStringSize.width(), line.height());
    painter->drawText(titleTextBox, Qt::AlignVCenter | Qt::TextWordWrap, track->getTitle());

    painter->restore();

}
