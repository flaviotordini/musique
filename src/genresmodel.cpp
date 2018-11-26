#include "genresmodel.h"

#include "model/decade.h"
#include "model/genre.h"
#include "model/item.h"

#include "finderwidget.h"
#include "trackmimedata.h"

namespace {

Finder::FinderItemTypes itemType(Item *item) {
    const char *type = item->metaObject()->className();
    if (type == QLatin1String("Genre")) return Finder::ItemTypeGenre;
    return Finder::ItemTypeDecade;
}

} // namespace

GenresModel::GenresModel(QObject *parent) : QAbstractItemModel(parent) {
    connect(&genres, &Genres::initializing, this, [this] { beginResetModel(); });
    connect(&genres, &Genres::initialized, this, [this] { endResetModel(); });
    genres.init();
}

int GenresModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        // Genre *parentGenre = parent.data(Finder::DataObjectRole).value<GenrePointer>().data();
        // if (parentGenre) return parentGenre->getChildren().size();
        Genre *parentItem = static_cast<Genre *>(parent.internalPointer());
        return parentItem->getChildren().size();
    } else
        return genres.getItems().size();
}

QVariant GenresModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    if (row < 0) return QVariant();

    Item *item = itemForIndex(index);
    if (!item) return QVariant();

    switch (role) {
    case Finder::ItemTypeRole:
        return itemType(item);

    case Finder::ItemObjectRole:
        return QVariant::fromValue(QPointer<Item>(item));

    case Finder::DataObjectRole:
        if (itemType(item) == Finder::ItemTypeGenre)
            return QVariant::fromValue(QPointer<Genre>(qobject_cast<Genre *>(item)));
        return QVariant::fromValue(QPointer<Decade>(qobject_cast<Decade *>(item)));
    }

    return QVariant();
}

QModelIndex GenresModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    if (!parent.isValid()) {
        Genre *childItem = qobject_cast<Genre *>(genres.getItems().at(row));
        if (!childItem) createIndex(row, column);
        return createIndex(row, column, childItem);
    }

    Genre *parentItem = static_cast<Genre *>(parent.internalPointer());
    if (!parentItem) return QModelIndex();

    Genre *childItem = parentItem->getChildren().at(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex GenresModel::parent(const QModelIndex &index) const {
    if (!index.isValid()) return QModelIndex();

    Genre *childItem = static_cast<Genre *>(index.internalPointer());
    if (!childItem) return QModelIndex();

    Genre *parentItem = childItem->getParent();
    if (parentItem == nullptr) return QModelIndex();

    int row = parentItem->getRow();
    return createIndex(row, 0, parentItem);
}

int GenresModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

// --- Sturm und drang ---

Qt::DropActions GenresModel::supportedDropActions() const {
    return Qt::CopyAction;
}

Qt::ItemFlags GenresModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return (defaultFlags | Qt::ItemIsDragEnabled);
    } else
        return defaultFlags;
}

QStringList GenresModel::mimeTypes() const {
    return TrackMimeData::types();
}

QMimeData *GenresModel::mimeData(const QModelIndexList &indexes) const {
    TrackMimeData *mime = new TrackMimeData();

    for (const QModelIndex &index : indexes) {
        Item *item = itemForIndex(index);
        if (item) mime->addTracks(item->getTracks());
    }

    return mime;
}

// ---

Item *GenresModel::itemForIndex(const QModelIndex &index) const {
    Item *item = nullptr;
    if (index.parent().isValid()) {
        item = static_cast<Item *>(index.internalPointer());
    } else {
        const int row = index.row();
        if (row < genres.getItems().size()) item = genres.getItems().at(row);
    }
    return item;
}
