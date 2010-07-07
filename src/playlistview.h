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
    void setEmptyPlaylistMessage(QString emptyMessage) {
        this->emptyMessage = emptyMessage;
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
    void paintEvent(QPaintEvent *event);
    /*
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    */

private:
    PlaylistModel *playlistModel;
    DropArea *dropArea;
    bool willHideDropArea;
    QString emptyMessage;
    QLabel* overlayLabel;

};

#endif // PLAYLISTVIEW_H
