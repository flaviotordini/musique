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
    void saveSplitterState();
    void setMediaObject(Phonon::MediaObject *mediaObject);
    Track* getActiveTrack() { return activeTrack; }
    PlaylistModel* getPlaylistModel() { return playlistModel; }

public slots:
    void appear();
    void disappear();
    void playPause();
    void trackRemoved();
    void search(QString query);

private slots:
    void activeRowChanged(int row, bool manual, bool startPlayback);
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void handleError(QString message);
    void playlistFinished();
    void playbackFinished();
    void aboutToFinish();
    void currentSourceChanged(Phonon::MediaSource mediaSource);
#ifdef APP_ACTIVATION
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

#ifdef APP_ACTIVATION
    void demoMessage();
    int tracksPlayed;
#endif

};

#endif // MEDIAVIEW_H
