#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QtGui>

class PlaylistModel;
class Track;

class PlaylistView : public QListView {

    Q_OBJECT

public:
    PlaylistView(QWidget *parent);
    void setPlaylistModel(PlaylistModel *model);

public slots:
    void removeSelected();
    void moveUpSelected();
    void moveDownSelected();
    void itemActivated(const QModelIndex &index);
    void selectionChanged (const QItemSelection & selected, const QItemSelection & deselected);
    void updatePlaylistActions();
    void selectTracks(QList<Track*> tracks);

protected:
    void dragEnterEvent(QDragEnterEvent *event);

private:
    PlaylistModel *playlistModel;

};

#endif // PLAYLISTVIEW_H
