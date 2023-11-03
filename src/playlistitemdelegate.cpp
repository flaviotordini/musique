#include "playlistitemdelegate.h"

#include "iconutils.h"

#include "model/album.h"
#include "model/artist.h"
#include "model/track.h"

#include "playlistmodel.h"
#include "playlistview.h"

namespace {
static const int PADDING = 10;
static int ITEM_HEIGHT = 0;

void drawElidedText(QPainter *painter, const QRect &textBox, const int flags, const QString &text) {
    QString elidedText =
            QFontMetrics(painter->font()).elidedText(text, Qt::ElideRight, textBox.width(), flags);
    painter->drawText(textBox, 0, elidedText);
}

} // namespace

PlaylistItemDelegate::PlaylistItemDelegate(PlaylistView *parent)
    : QStyledItemDelegate(parent), view(parent) {}

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    // determine item height based on font metrics
    ITEM_HEIGHT = option.fontMetrics.height() * 2;

    QModelIndex previousIndex = index.sibling(index.row() - 1, index.column());
    if (previousIndex.isValid()) {
        Track *previousTrack =
                previousIndex.data(PlaylistRoles::DataObjectRole).value<TrackPointer>();
        if (previousTrack) {
            Track *track = index.data(PlaylistRoles::DataObjectRole).value<TrackPointer>();
            Album *previousAlbum = previousTrack->getAlbum();
            Album *album = track->getAlbum();
            if (previousAlbum != album) return QSize(ITEM_HEIGHT, ITEM_HEIGHT * 2);
        }
    } else {
        return QSize(ITEM_HEIGHT, ITEM_HEIGHT*2);
    }

    return QSize(ITEM_HEIGHT, ITEM_HEIGHT);
}

void PlaylistItemDelegate::paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    paintTrack(painter, option, index);
}

void PlaylistItemDelegate::paintTrack(QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const {
    // get the data object
    Track *track = index.data(PlaylistRoles::DataObjectRole).value<TrackPointer>();

    // const PlaylistModel* playlistModel = dynamic_cast<const PlaylistModel*>(index.model());

    const bool isActive = index.data(PlaylistRoles::ActiveItemRole).toBool();
    // const bool isHovered = index.data(PlaylistRoles::HoveredItemRole).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    painter->save();

    painter->translate(option.rect.topLeft());
    QRect line(0, 0, option.rect.width(), option.rect.height());

    if (isSelected) {
        QRect selRect = line;
        selRect.setHeight(ITEM_HEIGHT);
        selRect.moveBottom(line.bottom());
        int selectedCount = view->selectionModel()->selectedIndexes().size();
        // force repaint of rounded selection
        if (selectedCount == 2) view->update();
        int radius = selectedCount > 1 ? 0 : 5;
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(option.palette.highlight());
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(selRect, radius, radius);
        painter->restore();
        painter->setPen(QPen(option.palette.brush(QPalette::HighlightedText), 0));
    } else
        painter->setPen(QPen(option.palette.brush(QPalette::Text), 0));

    if (line.height() > ITEM_HEIGHT) {
        // qDebug() << "header at index" << index.row();
        line.setHeight(ITEM_HEIGHT);
        paintAlbumHeader(painter, option, line, track);

        // now modify our rect and painter
        // to make them similar to "headerless" items
        line.moveBottom(ITEM_HEIGHT);
        painter->translate(0, ITEM_HEIGHT);
    }

    if (isActive) {
        auto icon = IconUtils::icon("media-playback-start", Qt::black);
        auto p = icon.pixmap(16);
        painter->drawPixmap(PADDING, (ITEM_HEIGHT - (p.height() / p.devicePixelRatio())) / 2, p);
    } else {
        paintTrackNumber(painter, option, line, track);
    }

    paintTrackTitle(painter, option, line, track);
    paintTrackLength(painter, option, line, track);

    painter->restore();
}

void PlaylistItemDelegate::paintAlbumHeader(QPainter* painter, const QStyleOptionViewItem& option,
                                            const QRect &line, Track* track) const {
    Album *album = track->getAlbum();
    Artist *artist = track->getArtist();

    int h = line.height();

    // cover
    Item *item = album;
    if (!item) item = artist;
    if (item) {
        const qreal pixelRatio = painter->device()->devicePixelRatioF();
        auto pixmap = item->getThumb(h, h, pixelRatio);
        painter->drawPixmap(0, 0, pixmap);
    }

    // title
    QString title;
    if (album) title = album->getName();
    if (artist) {
        if (!title.isEmpty()) title += " - ";
        title += artist->getName();
    }

    painter->save();
    painter->setPen(option.palette.color(QPalette::Text));
    painter->setOpacity(.75);

    auto fm = painter->fontMetrics();
    const int textLeft = h + fm.height() / 2;
    int width = fm.size(Qt::TextSingleLine, title).width();
    const int maxWidth = line.width() - textLeft;
    if (width > maxWidth) width = maxWidth;

    QRect textRect(textLeft, 0, width, line.height());
    title = fm.elidedText(title, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, title);
    painter->restore();
}

void PlaylistItemDelegate::paintTrackNumber(QPainter* painter, const QStyleOptionViewItem& option,
                                            const QRect &line, Track* track) const {

    const int trackNumber = track->getNumber();
    if (trackNumber < 1) return;

    painter->save();
    QFont font = painter->font();
    font.setPointSize(font.pointSize()-1);
    painter->setFont(font);
    QString trackString = QString("%1").arg(trackNumber, 2, 10, QChar('0'));
    QSize trackStringSize(QFontMetrics(painter->font()).size( Qt::TextSingleLine, trackString));
    QPoint textLoc(PADDING, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), trackStringSize.width(), line.height());

    painter->setOpacity(.5);
    painter->drawText(trackTextBox, Qt::AlignCenter, trackString);
    painter->restore();
}

void PlaylistItemDelegate::paintTrackTitle(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QRect &line, Track* track) const {

    QFontMetrics fontMetrics = QFontMetrics(painter->font());

    QString trackTitle = track->getTitle();

    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, trackTitle));
    QPoint textLoc(PADDING*4.5, 0);

    int width = trackStringSize.width();
    const int maxWidth = line.width() - 110;
    if (width > maxWidth) width = maxWidth;

    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    trackTitle = fontMetrics.elidedText(trackTitle, Qt::ElideRight, trackTextBox.width());

    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, trackTitle);

    // track artist
    if (trackTextBox.right() < maxWidth) {
        Album *album = track->getAlbum();
        if (album && album->getArtist()) {
            Artist *albumArtist = album->getArtist();
            Artist *trackArtist = track->getArtist();
            if (albumArtist && trackArtist && albumArtist->getId() != trackArtist->getId()) {
                static const QString by = "—";
                const int x = trackTextBox.right();
                QRect textBox(x, line.height(), 0, 0);
                const int flags = Qt::AlignVCenter | Qt::AlignLeft;
                QString artistName = track->getArtist()->getName();
                painter->save();
                textBox = painter->boundingRect(
                        line.adjusted(textBox.x() + textBox.width() + PADDING, 0, 0, 0), flags, by);
                if (textBox.right() > maxWidth) textBox.setRight(maxWidth);
                drawElidedText(painter, textBox, flags, by);
                textBox = painter->boundingRect(
                        line.adjusted(textBox.x() + textBox.width() + PADDING, 0, 0, 0), flags,
                        artistName);
                if (textBox.right() > maxWidth) textBox.setRight(maxWidth);
                drawElidedText(painter, textBox, flags, artistName);
                painter->restore();
            }
        }
    }
}

void PlaylistItemDelegate::paintTrackLength(QPainter* painter, const QStyleOptionViewItem& option,
                                            const QRect &line, Track* track) const {
    if (track->getLength() == 0) return;

    // QSize trackStringSize(QFontMetrics(painter->font()).size(Qt::TextSingleLine, trackLength));
    QPoint textLoc(PADDING*10, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), line.width() - textLoc.x() - PADDING, line.height());

    const bool isSelected = option.state & QStyle::State_Selected;
    painter->save();
    QFont font = painter->font();
    font.setPointSize(font.pointSize()-1);
    painter->setFont(font);
    if (isSelected)
        painter->setPen(option.palette.highlightedText().color());
    else
        painter->setOpacity(.5);
    painter->drawText(trackTextBox, Qt::AlignRight | Qt::AlignVCenter, track->getFormattedLength());
    painter->restore();

}
