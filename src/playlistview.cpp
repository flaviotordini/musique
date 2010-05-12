#include "playlistview.h"
#include "model/track.h"
#include "playlistmodel.h"
#include "playlistitemdelegate.h"

namespace The {
    QMap<QString, QAction*>* globalActions();
    QMap<QString, QMenu*>* globalMenus();
}

PlaylistView::PlaylistView(QWidget *parent) :
        QListView(parent),
        playlistModel(0) {

    // delegate
    setItemDelegate(new PlaylistItemDelegate(this));

    // cosmetics
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    // setAlternatingRowColors(true);

    // behaviour
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // dragndrop
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(false);
    setDragDropMode(QAbstractItemView::DragDrop);

    // actions
    connect(The::globalActions()->value("skip"), SIGNAL(triggered()), SLOT(skipForward()));
    connect(The::globalActions()->value("previous"), SIGNAL(triggered()), SLOT(skipBackward()));
    connect(The::globalActions()->value("remove"), SIGNAL(triggered()), SLOT(removeSelected()));
    connect(The::globalActions()->value("moveUp"), SIGNAL(triggered()), SLOT(moveUpSelected()));
    connect(The::globalActions()->value("moveDown"), SIGNAL(triggered()), SLOT(moveDownSelected()));

    // respond to the user doubleclicking a playlist item
    connect(this, SIGNAL(activated(const QModelIndex &)), SLOT(itemActivated(const QModelIndex &)));
}

void PlaylistView::setPlaylistModel(PlaylistModel *playlistModel) {

    this->playlistModel = playlistModel;
    setModel(playlistModel);

    // needed to restore the selection after dragndrop
    connect(playlistModel, SIGNAL(needSelectionFor(QList<Track*>)), SLOT(selectTracks(QList<Track*>)));

    connect(selectionModel(),
            SIGNAL(selectionChanged ( const QItemSelection & , const QItemSelection & )),
            SLOT(selectionChanged ( const QItemSelection & , const QItemSelection & )));

    connect(playlistModel,
            SIGNAL(dataChanged (const QModelIndex &, const QModelIndex &)),
            SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));

    connect(playlistModel, SIGNAL(modelReset()), SLOT(modelReset()));
    connect(The::globalActions()->value("clearPlaylist"), SIGNAL(triggered()), playlistModel, SLOT(clear()));
}

void PlaylistView::itemActivated(const QModelIndex &index) {
    if (playlistModel->rowExists(index.row()))
        playlistModel->setActiveRow(index.row());
}

void PlaylistView::skipBackward() {
    int previousRow = playlistModel->previousRow();
    if (previousRow == -1) return;
    playlistModel->setActiveRow(previousRow);
}

void PlaylistView::skipForward() {
    int nextRow = playlistModel->nextRow();
    if (nextRow == -1) return;
    playlistModel->setActiveRow(nextRow);
}

void PlaylistView::removeSelected() {
    if (!this->selectionModel()->hasSelection()) return;
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    playlistModel->removeIndexes(indexes);
}

void PlaylistView::selectTracks(QList<Track*> tracks) {
    foreach (Track *track, tracks) {
        QModelIndex index = playlistModel->indexForTrack(track);
        // FIXME this causes dropped tracks to disappear!
        // selectionModel()->select(index, QItemSelectionModel::Select);
        scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void PlaylistView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QListView::selectionChanged(selected, deselected);

    const bool gotSelection = this->selectionModel()->hasSelection();
    The::globalActions()->value("remove")->setEnabled(gotSelection);
    The::globalActions()->value("moveUp")->setEnabled(gotSelection);
    The::globalActions()->value("moveDown")->setEnabled(gotSelection);
}

void PlaylistView::modelReset() {
    The::globalActions()->value("clearPlaylist")->setEnabled(false);
}

void PlaylistView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
    QListView::dataChanged(topLeft, bottomRight);

    const int rowCount = playlistModel->rowCount(QModelIndex());
    // qDebug() << "Data changed" << rowCount;
    bool isPlaylistEmpty = rowCount < 1;
    The::globalActions()->value("clearPlaylist")->setEnabled(!isPlaylistEmpty);

    const int totalLength = playlistModel->getTotalLength();
    QString playlistLength;
    if (totalLength > 3600)
        playlistLength =  QTime().addSecs(totalLength).toString("h:mm:ss");
    else
        playlistLength = QTime().addSecs(totalLength).toString("m:ss");
    setStatusTip(tr("Playlist total length is %1").arg(playlistLength));
}

void PlaylistView::moveUpSelected() {
    if (!this->selectionModel()->hasSelection()) return;
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    playlistModel->move(indexes, true);

}

void PlaylistView::moveDownSelected() {
    if (!this->selectionModel()->hasSelection()) return;
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    playlistModel->move(indexes, false);
}
