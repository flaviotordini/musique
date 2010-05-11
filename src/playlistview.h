#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QtGui>
#include "model/track.h"
#include "playlistmodel.h"

class PlaylistView : public QListView {

    Q_OBJECT

public:
    PlaylistView(QWidget *parent);
    void setPlaylistModel(PlaylistModel *model);

public slots:
    void skipBackward();
    void skipForward();
    void removeSelected();
    void moveUpSelected();
    void moveDownSelected();
    void itemActivated(const QModelIndex &index);
    void selectionChanged (const QItemSelection & selected, const QItemSelection & deselected);
    void modelReset();
    void dataChanged(const QModelIndex &, const QModelIndex &);
    void selectTracks(QList<Track*> tracks);

private:
    PlaylistModel *playlistModel;

};

#endif // PLAYLISTVIEW_H
