/* $BEGIN_LICENSE

This file is part of Musique.
Copyright 2013, Flavio Tordini <flavio.tordini@gmail.com>

Musique is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Musique is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Musique.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <phonon/audiooutput.h>
#include <phonon/volumeslider.h>
#include <phonon/mediaobject.h>
#include <phonon/seekslider.h>

class MediaView;
class CollectionScannerView;
class ContextualView;
class SearchLineEdit;
class Track;
class UpdateChecker;
class Suggestion;

class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    static MainWindow* instance();
    ~MainWindow();
    QToolBar* getStatusToolbar() { return statusToolBar; }
    static void printHelp();

public slots:
    void showMediaView(bool transition = true);
    void showChooseFolderView(bool transition = true);
    void toggleContextualView();
    void hideContextualView();
    void updateContextualView(Track *track);
    void showInitialView();
    void quit();
    void showMessage(QString message);
    void handleError(QString message);
    void restore();
    void messageReceived(const QString &message);
    void goBack();
#ifdef APP_ACTIVATION
    void showActivationView(bool transition = true);
    void showActivationDialog();
    void buy();
    void hideBuyAction();
    void showDemoDialog(QString message);
#endif

protected:
    void closeEvent(QCloseEvent *);
    bool eventFilter(QObject *obj, QEvent *event);
    void resizeEvent(QResizeEvent *);

private slots:
    void lazyInit();
    void visitSite();
    void donate();
    void reportIssue();
    void about();
    void toggleFullscreen();
    void updateUIForFullscreen();
    void setShuffle(bool enabled);
    void setRepeat(bool enabled);
    void checkForUpdate();
    void gotNewVersion(QString version);

    // Phonon related logic
    void stop();
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void searchFocus();
    void tick(qint64 time);
    void totalTimeChanged(qint64 time);

    // volume shortcuts
    void volumeUp();
    void volumeDown();
    void volumeMute();
    void volumeChanged(qreal newVolume);
    void volumeMutedChanged(bool muted);

    // app logic
    void startFullScan(QString dir);
    void fullScanFinished();
    void startIncrementalScan();
    void incrementalScanProgress(int percent);
    void incrementalScanFinished();
    void startImageDownload();
    void imageDownloadFinished();
    void search(QString query);
    void suggestionAccepted(Suggestion *suggestion);
    void searchCleared();

    void showActionInStatusBar(QAction*, bool show);
    void showStopAfterThisInStatusBar(bool show);

    void toggleScrobbling(bool enable);
    void enableScrobbling();
    void disableScrobbling();
    void lastFmLogout();

    void savePlaylist();
    void loadPlaylist();

#ifdef APP_MAC_STORE
    void rateOnAppStore();
#endif

private:
    MainWindow();
    void showWidget(QWidget*, bool transition = true);
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    void initPhonon();
    static QString formatTime(qint64 time);
    QString playlistPath();
    void simpleUpdateDialog(QString version);

    // view mechanism
    QStackedWidget *views;
    QStack<QWidget*> *history;

    // view widgets
    QWidget *chooseFolderView;
    CollectionScannerView *collectionScannerView;
    MediaView *mediaView;
    ContextualView *contextualView;
    QWidget *aboutView;

    // actions
    QAction *contextualAct;
    QAction *backAct;
    QAction *quitAct;
    QAction *siteAct;
    QAction *donateAct;
    QAction *aboutAct;
    QAction *searchFocusAct;
    QAction *chooseFolderAct;

    // media actions
    QAction *skipBackwardAct;
    QAction *skipForwardAct;
    QAction *playAct;
    QAction *fullscreenAct;
    QAction *volumeUpAct;
    QAction *volumeDownAct;
    QAction *volumeMuteAct;

    // playlist actions
    QAction *removeAct;
    QAction *moveDownAct;
    QAction *moveUpAct;

    // menus
    QMenu *fileMenu;
    QMenu *playlistMenu;
    QMenu *playbackMenu;
    QMenu *helpMenu;

    // toolbar
    QToolBar *mainToolBar;
    SearchLineEdit *toolbarSearch;
    QToolBar *statusToolBar;

    // phonon
    Phonon::SeekSlider *seekSlider;
    Phonon::VolumeSlider *volumeSlider;
    Phonon::MediaObject *mediaObject;
    Phonon::AudioOutput *audioOutput;
    QLabel *currentTime;
    // QLabel *totalTime;

    // fullscreen
    bool m_fullscreen;
    bool m_maximized;

    // update checker
    UpdateChecker *updateChecker;

};

#endif
