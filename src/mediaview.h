#ifndef MEDIAVIEW_H
#define MEDIAVIEW_H

#include <QtGui>
#include <phonon/mediaobject.h>

#include "view.h"
#include "finderwidget.h"
#include "playlistview.h"
#include "playlistmodel.h"
#include "playlistwidget.h"

class MediaView : public QWidget, public View {

    Q_OBJECT;

public:
    MediaView(QWidget *parent);
    void appear();
    void disappear() {}
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", "");
        metadata.insert("description", "");
        return metadata;
    }
    void saveSplitterState();
    void setMediaObject(Phonon::MediaObject *mediaObject);
    PlaylistModel* getPlaylistModel() { return playlistModel; }

protected:
    void mouseMoveEvent (QMouseEvent *event);

private slots:
    void activeRowChanged(int);
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void handleError(QString message);

private:
    QSplitter *splitter;
    Phonon::MediaObject *mediaObject;
    FinderWidget *finderWidget;
    PlaylistView *playlistView;
    PlaylistModel *playlistModel;
    PlaylistWidget *playlistWidget;
    QTimer *errorTimer;
    QTimer *mouseTimer;

};

#endif // MEDIAVIEW_H
