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

#include <QtWidgets>

#include "media.h"

class ZoomableUI;
class Views;
class MediaView;
class CollectionScannerView;
class InfoView;
class SearchLineEdit;
class Track;
class UpdateChecker;
class Suggestion;
class ToolbarMenu;
class MessageBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    static MainWindow *instance();
    MainWindow();

    QSlider *getSeekSlider() { return seekSlider; }
    QSlider *getVolumeSlider() { return volumeSlider; }
    void readSettings();
    SearchLineEdit *getToolbarSearch() { return toolbarSearch; }
    QToolBar *getStatusToolbar() { return statusToolBar; }

    QAction *getAction(const char *name);
    void addNamedAction(const QByteArray &name, QAction *action);

    QMenu *getMenu(const char *name);

    Views *getViews() { return views; }

    static void printHelp();

public slots:
    void showMediaView();
    void showChooseFolderView();
    void toggleContextualView();
    void hideContextualView();
    void updateContextualView(Track *track);
    void showInitialView();
    void quit();
    void showMessage(const QString &message);
    void handleError(const QString &message);
    void restore();
    void messageReceived(const QString &message);
    void goBack();
    void visitSite();
    void donate();

signals:
    void currentTimeChanged(const QString &s);
    void collectionCreated();

protected:
    void closeEvent(QCloseEvent *e);
    bool eventFilter(QObject *obj, QEvent *e);
    void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *e);

private slots:
    void lazyInit();
    void reportIssue();
    void about();
    void toggleFullscreen();
    void updateUIForFullscreen();
    void setShuffle(bool enabled);
    void setRepeat(bool enabled);

    // media
    void stop();
    void stateChanged(Media::State state);
    void searchFocus();
    void tick(qint64 time);

    // volume shortcuts
    void volumeUp();
    void volumeDown();
    void toggleVolumeMute();
    void volumeChanged(qreal newVolume);
    void volumeMutedChanged(bool muted);

    // app logic
    void startFullScan(QString dir);
    void fullScanFinished(const QVariantMap &stats);
    void startIncrementalScan();
    void incrementalScanProgress(int percent);
    void incrementalScanFinished(const QVariantMap &stats);
    void search(QString query);
    void suggestionAccepted(Suggestion *suggestion);
    void searchCleared();

    void showActionInStatusBar(QAction *, bool show);
    void showStopAfterThisInStatusBar(bool show);

    void toggleScrobbling(bool enable);
    void enableScrobbling();
    void disableScrobbling();
    void lastFmLogout();

    void savePlaylist();
    void loadPlaylist();

    void toggleMenuVisibility();
    void toggleMenuVisibilityWithMessage();
    void toggleToolbarMenu();

#ifdef APP_MAC_STORE
    void rateOnAppStore();
#endif

    void runFinetune();
    void runFinetune(const QVariantMap &stats);
    void runFinetune(const QString &filename);

private:
    void showWidget(QWidget *widget);
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void writeSettings();
    void initMedia();
    QString playlistPath();
    void showFinetuneDialog(const QVariantMap &stats);

    Views *views;

    // view widgets
    QWidget *chooseFolderView;
    CollectionScannerView *collectionScannerView;
    MediaView *mediaView;
    InfoView *infoView;
    QWidget *aboutView;

    QHash<QByteArray, QAction *> actionMap;
    QHash<QByteArray, QMenu *> menuMap;

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

    ToolbarMenu *toolbarMenu;
    QToolButton *toolbarMenuButton;

    // toolbar
    QToolBar *toolbar;
    SearchLineEdit *toolbarSearch;
    QToolBar *statusToolBar;
    QSlider *seekSlider;
    QSlider *volumeSlider;
    QLabel *currentTimeLabel;

    // fullscreen
    bool fullScreenActive;
    bool maximizedBeforeFullScreen;
    bool menuVisibleBeforeFullScreen;

    Media *media;

    ZoomableUI *zoomableUI;
};

#endif
