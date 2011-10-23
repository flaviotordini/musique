#ifndef MEDIAVIEW_H
#define MEDIAVIEW_H

#include <QtGui>
#include <phonon/mediaobject.h>

#include "view.h"
#include "finderwidget.h"
#include "playlistview.h"
#include "playlistmodel.h"
#include "playlistwidget.h"

class DropArea;

class MediaView : public QWidget, public View {

    Q_OBJECT

public:
    MediaView(QWidget *parent);
    void disappear() {}
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", "");
        metadata.insert("description", "");
        return metadata;
    }
    void saveSplitterState();
    void setMediaObject(Phonon::MediaObject *mediaObject);
    Track* getActiveTrack() { return activeTrack; }
    PlaylistModel* getPlaylistModel() { return playlistModel; }

public slots:
    void appear();
    void playPause();
    void trackRemoved();
    void search(QString query);

private slots:
    void activeRowChanged(int row, bool manual);
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void handleError(QString message);
    void playlistFinished();
    void playbackFinished();
#ifdef APP_DEMO
    void updateContinueButton(int);
#endif

private:
    QSplitter *splitter;
    Phonon::MediaObject *mediaObject;
    FinderWidget *finderWidget;
    PlaylistView *playlistView;
    PlaylistModel *playlistModel;
    PlaylistArea *playlistWidget;
    QTimer *errorTimer;
    DropArea *dropArea;
    Track *activeTrack;

#ifdef APP_DEMO
    void demoMessage();
    int tracksPlayed;
#endif

};

#endif // MEDIAVIEW_H
