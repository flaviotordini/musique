#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
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
class FaderWidget;

class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    static MainWindow* instance();
    ~MainWindow();

public slots:
    void showMediaView();
    void showChooseFolderView();
    void toggleContextualView();
    void hideContextualView();
    void updateContextualView(Track *track);
    void showInitialView();
#ifdef APP_DEMO
    void showDemoDialog(QString message);
    void buy();
#endif

protected:
    void closeEvent(QCloseEvent *);
    bool eventFilter(QObject *obj, QEvent *event);
    void resizeEvent(QResizeEvent *);

private slots:
    void crossfadeViews(QWidget *oldWidget, QWidget *newWidget);
    void goBack();
    void visitSite();
    void donate();
    void about();
    void quit();
    void toggleFullscreen();
    void updateUIForFullscreen();
    void setShuffle(bool enabled);
    void setRepeat(bool enabled);
    void checkForUpdate();
    void gotNewVersion(QString version);
    void enableUnifiedToolbar() {
        setUnifiedTitleAndToolBarOnMac(true);
    }

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
    void search(QString query);
    void searchCleared();

private:
    MainWindow();
    void showView(QWidget*);
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    void initPhonon();
    static QString formatTime(qint64 time);
    QString playlistPath();
    void savePlaylist();
    void loadPlaylist();

    // view mechanism
    QPointer<FaderWidget> faderWidget;
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
    // QAction *stopAct;
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
    QLabel *totalTime;

    // fullscreen
    bool m_fullscreen;
    bool m_maximized;

    // update checker
    UpdateChecker *updateChecker;

};

#endif
