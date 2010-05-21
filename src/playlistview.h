#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QtGui>

class PlaylistModel;
class Track;
class DropArea;

class PlaylistView : public QListView {

    Q_OBJECT

public:
    PlaylistView(QWidget *parent);
    void setPlaylistModel(PlaylistModel *model);
    void setDropArea(DropArea *dropArea) {
        this->dropArea = dropArea;
    }

signals:
    void needDropArea();

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
    void dragLeaveEvent(QDragLeaveEvent *event);

private:
    PlaylistModel *playlistModel;
    DropArea *dropArea;
    bool willHideDropArea;

};

#endif // PLAYLISTVIEW_H
