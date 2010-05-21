#ifndef DROPAREA_H
#define DROPAREA_H

#include <QtGui>
class PlaylistModel;

class DropArea : public QLabel {
    Q_OBJECT

public:
    DropArea(QWidget *parent = 0);
    void setPlaylistModel(PlaylistModel *playlistModel) {
        this->playlistModel = playlistModel;
    }

public slots:
    void clear();

signals:
    void changed(const QMimeData *mimeData = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    PlaylistModel *playlistModel;

};

#endif
