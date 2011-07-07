#include "mainwindow.h"
#include "spacer.h"
#include "constants.h"
#include "iconloader/qticonloader.h"
#include "global.h"
#include "database.h"
#include <QtSql>
#include "contextualview.h"
#include "faderwidget/faderwidget.h"
#include "searchlineedit.h"
#include "view.h"
#include "mediaview.h"
#include "aboutview.h"
#include "choosefolderview.h"
#include "collectionscanner.h"
#include "collectionscannerview.h"
#include "updatechecker.h"
#include "fontutils.h"
#include "globalshortcuts.h"
#ifdef Q_WS_X11
#include "gnomeglobalshortcutbackend.h"
#endif
#ifdef QT_MAC_USE_COCOA
#include "local/mac/mac_startup.h"
#endif
#include "collectionsuggester.h"

/*
class CentralWidget : public QWidget {

public:
    CentralWidget(QWidget *message, QWidget *views, QWidget* parent) : QWidget(parent) {
        QBoxLayout *layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(message);
        layout->addWidget(views);
        setLayout(layout);
    }

};
*/

MainWindow::MainWindow() {
    m_fullscreen = false;

    // lazily initialized views
    mediaView = 0;
    collectionScannerView = 0;
    chooseFolderView = 0;
    aboutView = 0;
    contextualView = 0;

    toolbarSearch = new SearchLineEdit(this);
    toolbarSearch->setFont(qApp->font());
    toolbarSearch->setMinimumWidth(toolbarSearch->fontInfo().pixelSize()*15);
    toolbarSearch->setSuggester(new CollectionSuggester(this));
    connect(toolbarSearch, SIGNAL(search(const QString&)), SLOT(search(const QString&)));
    connect(toolbarSearch, SIGNAL(cleared()), SLOT(searchCleared()));

    // build ui
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    // views mechanism
    history = new QStack<QWidget*>();
    views = new QStackedWidget(this);

    // remove that useless menu/toolbar context menu
    setContextMenuPolicy(Qt::NoContextMenu);

    setCentralWidget(views);

    // restore window position
    readSettings();

    // show the initial view
    Database &db = Database::instance();
    if (db.status() == ScanComplete) {

        showMediaView();
        loadPlaylist();

        // update the collection when idle
        QTimer::singleShot(1000, this, SLOT(startIncrementalScan()));

        updateChecker = 0;
        checkForUpdate();

    } else {
        // no db, do the first scan dance
        showChooseFolderView();
    }

    // event filter to block ugly toolbar tooltips
    qApp->installEventFilter(this);

    // Global shortcuts
    GlobalShortcuts &shortcuts = GlobalShortcuts::instance();
#ifdef Q_WS_X11
    if (GnomeGlobalShortcutBackend::IsGsdAvailable())
        shortcuts.setBackend(new GnomeGlobalShortcutBackend(&shortcuts));
#endif
#ifdef QT_MAC_USE_COCOA
    mac::MacSetup();
#endif
    connect(&shortcuts, SIGNAL(PlayPause()), playAct, SLOT(trigger()));
    connect(&shortcuts, SIGNAL(Stop()), this, SLOT(stop()));
}

MainWindow::~MainWindow() {
    delete history;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::ToolTip) {
        // kill tooltips
        return true;
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        // qDebug() << keyEvent;
        if (keyEvent->key() == Qt::Key_MediaStop) {
            qDebug() << "Stop!";
            return false;
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);

}

void MainWindow::createActions() {

    QMap<QString, QAction*> *actions = The::globalActions();

    backAct = new QAction(tr("&Back"), this);
    backAct->setEnabled(false);
    backAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    backAct->setStatusTip(tr("Go to the previous view"));
    actions->insert("back", backAct);
    connect(backAct, SIGNAL(triggered()), SLOT(goBack()));

    QIcon icon = QtIconLoader::icon("gtk-info");
#ifdef Q_WS_X11
    if (icon.isNull()) {
        icon = QtIconLoader::icon("help-about");
    }
#endif
    contextualAct = new QAction(icon, tr("&Info"), this);
    contextualAct->setStatusTip(tr("Show information about the current track"));
    contextualAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return));
    contextualAct->setEnabled(false);
    contextualAct->setCheckable(true);
    actions->insert("contextual", contextualAct);
    connect(contextualAct, SIGNAL(triggered()), SLOT(toggleContextualView()));

    /*
    stopAct = new QAction(
            QtIconLoader::icon("media-playback-stop"),
            tr("&Stop"), this);
    stopAct->setStatusTip(tr("Stop playback and go back to the search view"));
    stopAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::Key_Escape) << QKeySequence(Qt::Key_MediaStop));
    stopAct->setEnabled(false);
    actions->insert("stop", stopAct);
    connect(stopAct, SIGNAL(triggered()), SLOT(stop()));
    */

    skipBackwardAct = new QAction(
                QtIconLoader::icon("media-skip-backward"),
                tr("P&revious"), this);
    skipBackwardAct->setStatusTip(tr("Go back to the previous track"));
    skipBackwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
#if QT_VERSION >= 0x040600
    skipBackwardAct->setPriority(QAction::LowPriority);
#endif
    skipBackwardAct->setEnabled(false);
    actions->insert("previous", skipBackwardAct);

    skipForwardAct = new QAction(
                QtIconLoader::icon("media-skip-forward"),
                tr("&Next"), this);
    skipForwardAct->setStatusTip(tr("Skip to the next track"));
    skipForwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
#if QT_VERSION >= 0x040600
    skipForwardAct->setPriority(QAction::LowPriority);
#endif
    skipForwardAct->setEnabled(false);
    actions->insert("skip", skipForwardAct);

    playAct = new QAction(
                QtIconLoader::icon("media-playback-start"),
                tr("&Play"), this);
    playAct->setStatusTip(tr("Start playback"));
    playAct->setShortcuts(QList<QKeySequence>()
                          << QKeySequence(Qt::Key_Space)
                          << QKeySequence(Qt::Key_MediaPlay));
    playAct->setEnabled(false);
    playAct->setCheckable(true);
#ifdef APP_MAC
    playAct->setPriority(QAction::LowPriority);
#endif
    actions->insert("play", playAct);

#ifndef APP_MAC_NO
    fullscreenAct = new QAction(
                QtIconLoader::icon("view-restore"),
                tr("&Full Screen"), this);
    fullscreenAct->setStatusTip(tr("Go full screen"));
    fullscreenAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Return));
    fullscreenAct->setShortcuts(QList<QKeySequence>()
                                << QKeySequence(Qt::ALT + Qt::Key_Return)
                                << QKeySequence(Qt::Key_F11));
    fullscreenAct->setShortcutContext(Qt::ApplicationShortcut);
    actions->insert("fullscreen", fullscreenAct);
    connect(fullscreenAct, SIGNAL(triggered()), SLOT(toggleFullscreen()));
#endif

    removeAct = new QAction(tr("&Remove"), this);
    removeAct->setStatusTip(tr("Remove the selected tracks from the playlist"));
    removeAct->setShortcuts(QList<QKeySequence>() << QKeySequence("Del") << QKeySequence("Backspace"));
    removeAct->setEnabled(false);
    actions->insert("remove", removeAct);

    moveUpAct = new QAction(tr("Move &Up"), this);
    moveUpAct->setStatusTip(tr("Move up the selected tracks in the playlist"));
    moveUpAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    moveUpAct->setEnabled(false);
    actions->insert("moveUp", moveUpAct);

    moveDownAct = new QAction(tr("Move &Down"), this);
    moveDownAct->setStatusTip(tr("Move down the selected tracks in the playlist"));
    moveDownAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    moveDownAct->setEnabled(false);
    actions->insert("moveDown", moveDownAct);

    quitAct = new QAction(tr("&Quit"), this);
    quitAct->setMenuRole(QAction::QuitRole);
    quitAct->setShortcut(tr("Ctrl+Q"));
    quitAct->setStatusTip(tr("Bye"));
    actions->insert("quit", quitAct);
    connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));

    chooseFolderAct = new QAction(tr("&Change collection folder..."), this);
    chooseFolderAct->setStatusTip(tr("Choose a different music collection folder"));
    chooseFolderAct->setMenuRole(QAction::ApplicationSpecificRole);
    actions->insert("chooseFolder", chooseFolderAct);
    connect(chooseFolderAct, SIGNAL(triggered()), SLOT(showChooseFolderView()));

    siteAct = new QAction(tr("&Website"), this);
    siteAct->setShortcut(QKeySequence::HelpContents);
    siteAct->setStatusTip(tr("%1 on the Web").arg(Constants::APP_NAME));
    actions->insert("site", siteAct);
    connect(siteAct, SIGNAL(triggered()), SLOT(visitSite()));

    donateAct = new QAction(tr("Make a &donation"), this);
    donateAct->setStatusTip(tr("Please support the continued development of %1").arg(Constants::APP_NAME));
    actions->insert("donate", donateAct);
    connect(donateAct, SIGNAL(triggered()), SLOT(donate()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setMenuRole(QAction::AboutRole);
    aboutAct->setStatusTip(tr("Info about %1").arg(Constants::APP_NAME));
    actions->insert("about", aboutAct);
    connect(aboutAct, SIGNAL(triggered()), SLOT(about()));

    // Anon
    QAction *action;

    action = new QAction(QtIconLoader::icon("edit-clear"), tr("&Clear"), this);
    action->setShortcut(QKeySequence::New);
    action->setStatusTip(tr("Remove all tracks from the playlist"));
    action->setEnabled(false);
    actions->insert("clearPlaylist", action);

    action = new QAction(
                QtIconLoader::icon("media-playlist-shuffle"), tr("&Shuffle"), this);
    action->setStatusTip(tr("Random playlist mode"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), SLOT(setShuffle(bool)));
    actions->insert("shufflePlaylist", action);

    action = new QAction(
                QtIconLoader::icon("media-playlist-repeat"),
                tr("&Repeat"), this);
    action->setStatusTip(tr("Play first song again after all songs are played"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), SLOT(setRepeat(bool)));
    actions->insert("repeatPlaylist", action);

    // Invisible actions

    searchFocusAct = new QAction(this);
    searchFocusAct->setShortcut(QKeySequence::Find);
    actions->insert("search", searchFocusAct);
    connect(searchFocusAct, SIGNAL(triggered()), SLOT(searchFocus()));
    addAction(searchFocusAct);

    volumeUpAct = new QAction(this);
    volumeUpAct->setShortcuts(QList<QKeySequence>()
                              << QKeySequence(Qt::CTRL + Qt::Key_Plus)
                              << QKeySequence(Qt::Key_VolumeUp));
    actions->insert("volume-up", volumeUpAct);
    connect(volumeUpAct, SIGNAL(triggered()), SLOT(volumeUp()));
    addAction(volumeUpAct);

    volumeDownAct = new QAction(this);
    volumeDownAct->setShortcuts(QList<QKeySequence>()
                                << QKeySequence(Qt::CTRL + Qt::Key_Minus)
                                << QKeySequence(Qt::Key_VolumeDown));
    actions->insert("volume-down", volumeDownAct);
    connect(volumeDownAct, SIGNAL(triggered()), SLOT(volumeDown()));
    addAction(volumeDownAct);

    volumeMuteAct = new QAction(this);
    volumeMuteAct->setIcon(QtIconLoader::icon("audio-volume-high"));
    volumeMuteAct->setStatusTip(tr("Mute volume"));
    volumeMuteAct->setShortcuts(QList<QKeySequence>()
                                << QKeySequence(tr("Ctrl+M"))
                                << QKeySequence(Qt::Key_VolumeMute));
    actions->insert("volume-mute", volumeMuteAct);
    connect(volumeMuteAct, SIGNAL(triggered()), SLOT(volumeMute()));
    addAction(volumeMuteAct);

    // common action properties
    foreach (QAction *action, actions->values()) {

        // add actions to the MainWindow so that they work
        // when the menu is hidden
        addAction(action);

        // never autorepeat.
        // unexperienced users tend to keep keys pressed for a "long" time
        action->setAutoRepeat(false);

        // not needed since we disabled tooltips altogether
        // action->setToolTip(action->statusTip());

        // make the actions work when in fullscreen
        action->setShortcutContext(Qt::ApplicationShortcut);

        // show keyboard shortcuts in the status bar
        if (!action->shortcut().isEmpty())
            action->setStatusTip(action->statusTip() +
                                 " (" + action->shortcut().toString(QKeySequence::NativeText) + ")");

        // no icons in menus
        action->setIconVisibleInMenu(false);

    }

}

void MainWindow::createMenus() {

    QMap<QString, QMenu*> *menus = The::globalMenus();

    fileMenu = menuBar()->addMenu(tr("&Application"));
    fileMenu->addAction(chooseFolderAct);
#ifndef APP_MAC
    fileMenu->addSeparator();
    fileMenu->addAction(quitAct);
#endif

    playbackMenu = menuBar()->addMenu(tr("&Playback"));
    menus->insert("playback", playbackMenu);
    // playbackMenu->addAction(stopAct);
    playbackMenu->addAction(playAct);
    playbackMenu->addSeparator();
    playbackMenu->addAction(skipForwardAct);
    playbackMenu->addAction(skipBackwardAct);
#ifdef APP_MAC
    extern void qt_mac_set_dock_menu(QMenu *);
    qt_mac_set_dock_menu(playbackMenu);
#endif

    playlistMenu = menuBar()->addMenu(tr("Play&list"));
    menus->insert("playlist", playlistMenu);
    playlistMenu->addAction(The::globalActions()->value("clearPlaylist"));
    playlistMenu->addAction(The::globalActions()->value("shufflePlaylist"));
    playlistMenu->addAction(The::globalActions()->value("repeatPlaylist"));
    playlistMenu->addSeparator();
    playlistMenu->addAction(removeAct);
    playlistMenu->addAction(moveUpAct);
    playlistMenu->addAction(moveDownAct);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(contextualAct);
#ifndef APP_MAC_NO
    viewMenu->addSeparator();
    viewMenu->addAction(fullscreenAct);
#endif

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(siteAct);
#if !defined(APP_MAC) && !defined(APP_WIN)
    helpMenu->addAction(donateAct);
#endif
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars() {

    setUnifiedTitleAndToolBarOnMac(true);
    mainToolBar = new QToolBar(this);
#if QT_VERSION < 0x040600
    mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#elif defined(APP_WIN)
    mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mainToolBar->setStyleSheet(
            "QToolBar {"
                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fff, stop:.5 #ececec, stop:.51 #e0e0e0, stop:1 #ccc);"
                "border: 0;"
                "border-bottom: 1px solid #a0afc3;"
            "}");
#else
    mainToolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#endif
    mainToolBar->setFloatable(false);
    mainToolBar->setMovable(false);

#if defined(APP_MAC) | defined(APP_WIN)
    mainToolBar->setIconSize(QSize(32, 32));
#endif

    mainToolBar->addAction(skipBackwardAct);
    mainToolBar->addAction(playAct);
    mainToolBar->addAction(skipForwardAct);

    mainToolBar->addAction(contextualAct);

    mainToolBar->addWidget(new Spacer());

    QFont smallerFont = FontUtils::small();
    currentTime = new QLabel(mainToolBar);
    currentTime->setFont(smallerFont);
    mainToolBar->addWidget(currentTime);

    mainToolBar->addWidget(new Spacer());

    seekSlider = new Phonon::SeekSlider(this);
    seekSlider->setIconVisible(false);
    seekSlider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mainToolBar->addWidget(seekSlider);

    mainToolBar->addWidget(new Spacer());

    totalTime = new QLabel(mainToolBar);
    totalTime->setFont(smallerFont);
    mainToolBar->addWidget(totalTime);

    mainToolBar->addWidget(new Spacer());

    mainToolBar->addAction(volumeMuteAct);

    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setMuteVisible(false);

    // qDebug() << volumeSlider->children();
    // status tip for the volume slider
    QSlider* volumeQSlider = volumeSlider->findChild<QSlider*>();
    if (volumeQSlider)
        volumeQSlider->setStatusTip(tr("Press %1 to raise the volume, %2 to lower it").arg(
                                        volumeUpAct->shortcut().toString(QKeySequence::NativeText),
                                        volumeDownAct->shortcut().toString(QKeySequence::NativeText)));

    // this makes the volume slider smaller
    volumeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    mainToolBar->addWidget(volumeSlider);

    mainToolBar->addWidget(new Spacer());

    mainToolBar->addWidget(toolbarSearch);

    addToolBar(mainToolBar);
}

void MainWindow::createStatusBar() {

    // remove ugly borders on OSX
    statusBar()->setStyleSheet("::item{border:0 solid} QToolBar {padding:0;spacing:0;margin:0;border:0}");

    statusToolBar = new QToolBar(this);
    statusToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

#ifdef APP_MAC
    int iconHeight = 17;
#else
    int iconHeight = 24;
#endif
    int iconWidth = iconHeight * 3 / 2;
    statusToolBar->setIconSize(QSize(iconWidth, iconHeight));
    statusToolBar->addAction(The::globalActions()->value("shufflePlaylist"));
    statusToolBar->addAction(The::globalActions()->value("repeatPlaylist"));
    statusToolBar->addAction(The::globalActions()->value("clearPlaylist"));
    statusBar()->addPermanentWidget(statusToolBar);

    statusBar()->show();
}

void MainWindow::readSettings() {
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
#ifdef APP_MAC
    if (!isMaximized())
#ifdef QT_MAC_USE_COCOA
        move(x(), y() + 10);
#else
        move(x(), y() + mainToolBar->height() + 8);
#endif
#endif
    m_maximized = isMaximized();
    The::globalActions()->value("shufflePlaylist")->setChecked(settings.value("shuffle").toBool());
    The::globalActions()->value("repeatPlaylist")->setChecked(settings.value("repeat").toBool());
}

void MainWindow::writeSettings() {

    QSettings settings;

    // do not save geometry when in full screen
    if (!m_fullscreen)
        settings.setValue("geometry", saveGeometry());

    if (mediaView) {
        settings.setValue("volume", audioOutput->volume());
        settings.setValue("volumeMute", audioOutput->isMuted());
        mediaView->saveSplitterState();
    }

}

void MainWindow::goBack() {
    if (history->size() > 1) {
        history->pop();
        QWidget *widget = history->pop();
        showView(widget);
    }
}

void MainWindow::showView(QWidget* widget) {

    setUpdatesEnabled(false);

    // call disappear() method on the old view
    View* oldView = dynamic_cast<View *> (views->currentWidget());
    if (oldView) {
        oldView->disappear();
    }

    // call appear() method on the new view
    View* newView = dynamic_cast<View *> (widget);
    if (newView) {
        newView->appear();

        QMap<QString,QVariant> metadata = newView->metadata();
        /*
        QString windowTitle = metadata.value("title").toString();
        if (windowTitle.length())
            windowTitle += " - ";
        setWindowTitle(windowTitle + Constants::APP_NAME);
        */
        statusBar()->showMessage((metadata.value("description").toString()));
    }


    fullscreenAct->setEnabled(widget == mediaView || widget == contextualView);
    aboutAct->setEnabled(widget != aboutView);
    chooseFolderAct->setEnabled(widget != chooseFolderView && widget != collectionScannerView);

    // toolbar only for the mediaView
    /*
    bool showBars = widget == mediaView || widget == contextualView;
    mainToolBar->setVisible(showBars);
    statusBar()->setVisible(showBars);
    */

    mainToolBar->setVisible(true);
    statusBar()->setVisible(true);
#if defined(APP_MAC) || defined(APP_WIN)
    // crossfade only on OSX
    // where we can be sure of video performance
    // crossfadeViews(views->currentWidget(), widget);
#endif

    views->setCurrentWidget(widget);

    setUpdatesEnabled(true);

    history->push(widget);

}

void MainWindow::crossfadeViews(QWidget *oldWidget, QWidget *newWidget) {
    /*
    qDebug() << "MainWindow::crossfadeViews";
    qDebug() << "views" << oldWidget << newWidget;
    if (!oldWidget || !newWidget) {
        qDebug() << "no widgets";
        return;
    }
    // if (faderWidget) faderWidget->close();
    FaderWidget *faderWidget = new FaderWidget(oldWidget, newWidget);
    faderWidget->start();*/

    if (!oldWidget || !newWidget) {
        // qDebug() << "no widgets";
        return;
    }
    FaderWidget *faderWidget = new FaderWidget(newWidget);
    faderWidget->start(QPixmap::grabWidget(oldWidget));
}

void MainWindow::about() {
    if (!aboutView) {
        aboutView = new AboutView(this);
        views->addWidget(aboutView);
    }
    showView(aboutView);
}

void MainWindow::visitSite() {
    QUrl url(Constants::WEBSITE);
    statusBar()->showMessage(QString(tr("Opening %1").arg(url.toString())));
    QDesktopServices::openUrl(url);
}

void MainWindow::donate() {
    QUrl url(QString(Constants::WEBSITE) + "#donate");
    statusBar()->showMessage(QString(tr("Opening %1").arg(url.toString())));
    QDesktopServices::openUrl(url);
}

void MainWindow::quit() {
    savePlaylist();
    writeSettings();
    /*
    CollectionScanner *scanner = CollectionScanner::instance();
    scanner->stop();
    if (scanner->thread() != thread()) {
        // scanner->thread()->wait();
    }*/
    delete &Database::instance();
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    quit();
    QMainWindow::closeEvent(event);
}

void MainWindow::showChooseFolderView() {
    if (!chooseFolderView) {
        chooseFolderView = new ChooseFolderView(this);
        connect(chooseFolderView, SIGNAL(locationChanged(QString)), SLOT(startFullScan(QString)));
        views->addWidget(chooseFolderView);
    }
    showView(chooseFolderView);
}

void MainWindow::showMediaView() {
    if(!mediaView) {
        initPhonon();
        mediaView = new MediaView(this);
        mediaView->setMediaObject(mediaObject);
        connect(playAct, SIGNAL(triggered()), mediaView, SLOT(playPause()));
        views->addWidget(mediaView);
    }

    if (views->currentWidget() == contextualView) {
        hideContextualView();
    }

    mediaView->setFocus();
    showView(mediaView);
}

void MainWindow::toggleContextualView() {
    if (!contextualView) {
        contextualView = new ContextualView(this);
        views->addWidget(contextualView);
    }
    if (views->currentWidget() == contextualView) {
        hideContextualView();
    } else {
        Track *track = mediaView->getActiveTrack();
        if (track) {
            contextualView->setTrack(track);
            showView(contextualView);
            contextualAct->setChecked(true);

            // stopAct->setShortcut(QString(""));
            QList<QKeySequence> shortcuts;
            // for some reason it is important that ESC comes first
            shortcuts << QKeySequence(Qt::Key_Escape) << contextualAct->shortcuts();
            contextualAct->setShortcuts(shortcuts);
        }
    }
}

void MainWindow::hideContextualView() {
    goBack();
    contextualAct->setChecked(false);

    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(Qt::CTRL + Qt::Key_Return);
    contextualAct->setShortcuts(shortcuts);
    // stopAct->setShortcut(QKeySequence(Qt::Key_Escape));
}

void MainWindow::updateContextualView(Track *track) {
    if (views->currentWidget() == contextualView) {
        contextualView->setTrack(track);
    }
}

void MainWindow::startFullScan(QString directory) {
    // qDebug() << "startFullScan" << directory;

    if (!collectionScannerView) {
        collectionScannerView = new CollectionScannerView(this);
        views->addWidget(collectionScannerView);
    }
    showView(collectionScannerView);

    CollectionScannerThread *scannerThread = new CollectionScannerThread();
    collectionScannerView->setCollectionScannerThread(scannerThread);
    scannerThread->setDirectory(directory);
    connect(scannerThread, SIGNAL(finished()), SLOT(fullScanFinished()));
    scannerThread->start();

    if (mediaView) {
        stop();
        PlaylistModel* playlistModel = mediaView->getPlaylistModel();
        if (playlistModel) {
            playlistModel->clear();
        }
    }
}

void MainWindow::fullScanFinished() {
    QApplication::alert(this, 0);
}

void MainWindow::startIncrementalScan() {
    statusBar()->showMessage(tr("Updating collection..."));
    chooseFolderAct->setEnabled(false);
    CollectionScannerThread *scannerThread = new CollectionScannerThread();
    // incremental!
    scannerThread->setDirectory(QString());
    connect(scannerThread, SIGNAL(progress(int)), SLOT(incrementalScanProgress(int)));
    connect(scannerThread, SIGNAL(finished()), SLOT(incrementalScanFinished()));
    scannerThread->start();
}

void MainWindow::incrementalScanProgress(int percent) {
    statusBar()->showMessage(tr("Updating collection - %1%").arg(QString::number(percent)));
}

void MainWindow::incrementalScanFinished() {
    chooseFolderAct->setEnabled(true);
    statusBar()->showMessage(tr("Collection updated"));
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */) {

    // qDebug() << "Phonon state: " << newState;

    // play action
    if (newState == Phonon::PlayingState) {
        playAct->setChecked(true);
    } else if (newState == Phonon::StoppedState || newState == Phonon::PausedState ) {
        playAct->setChecked(false);
    }

    switch (newState) {

    case Phonon::ErrorState:
        if (mediaObject->errorType() == Phonon::FatalError) {
            statusBar()->showMessage(tr("Fatal error: %1").arg(mediaObject->errorString()));
        } else {
            statusBar()->showMessage(tr("Error: %1").arg(mediaObject->errorString()));
        }
        break;

    case Phonon::PlayingState:
        // stopAct->setEnabled(true);
        contextualAct->setEnabled(true);
        break;

    case Phonon::StoppedState:
        // stopAct->setEnabled(false);
        contextualAct->setEnabled(false);
        break;

    case Phonon::PausedState:
        // stopAct->setEnabled(true);
        break;

    case Phonon::BufferingState:
    case Phonon::LoadingState:
        // stopAct->setEnabled(true);
        currentTime->clear();
        totalTime->clear();
        break;

    default:
        contextualAct->setEnabled(false);
    }
}

void MainWindow::stop() {
    mediaObject->stop();
    currentTime->clear();
    totalTime->clear();
}

void MainWindow::toggleFullscreen() {

    // setUpdatesEnabled(false);

    if (m_fullscreen) {
        fullscreenAct->setShortcuts(QList<QKeySequence>()
                                    << QKeySequence(Qt::ALT + Qt::Key_Return)
                                    << QKeySequence(Qt::Key_F11));
        fullscreenAct->setText(tr("&Full Screen"));
        // stopAct->setShortcut(QKeySequence(Qt::Key_Escape));

        mainToolBar->removeAction(fullscreenAct);

#if APP_MAC
        setCentralWidget(views);
        views->showNormal();
        show();
        mediaView->setFocus();
#else
        // mainToolBar->show();
        if (m_maximized) showMaximized();
        else showNormal();
#endif

        activateWindow();

    } else {
        // stopAct->setShortcut(QString(""));
        fullscreenAct->setShortcuts(QList<QKeySequence>()
                                    << QKeySequence(Qt::Key_Escape)
                                    << QKeySequence(Qt::ALT + Qt::Key_Return)
                                    << QKeySequence(Qt::Key_F11));
        fullscreenAct->setText(tr("Leave &Full Screen"));
        m_maximized = isMaximized();

        // save geometry now, if the user quits when in full screen
        // geometry won't be saved
        writeSettings();

        mainToolBar->addAction(fullscreenAct);

#ifdef APP_MAC
        hide();
        views->setParent(0);
        QTimer::singleShot(0, views, SLOT(showFullScreen()));
#else
        // mainToolBar->hide();
        showFullScreen();
#endif

    }

#ifndef APP_MAC
    menuBar()->setVisible(m_fullscreen);
#endif

    m_fullscreen = !m_fullscreen;

    // setUpdatesEnabled(true);
}

void MainWindow::searchFocus() {
    toolbarSearch->selectAll();
    toolbarSearch->setFocus();
}

void MainWindow::initPhonon() {
    mediaObject = new Phonon::MediaObject(this);
    mediaObject->setTickInterval(100);
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            this, SLOT(stateChanged(Phonon::State, Phonon::State)));
    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(mediaObject, SIGNAL(totalTimeChanged(qint64)), this, SLOT(totalTimeChanged(qint64)));

    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    connect(audioOutput, SIGNAL(volumeChanged(qreal)), this, SLOT(volumeChanged(qreal)));
    connect(audioOutput, SIGNAL(mutedChanged(bool)), this, SLOT(volumeMutedChanged(bool)));
    Phonon::createPath(mediaObject, audioOutput);

    seekSlider->setMediaObject(mediaObject);
    volumeSlider->setAudioOutput(audioOutput);

    QSettings settings;
    audioOutput->setVolume(settings.value("volume", 1).toDouble());
    audioOutput->setMuted(settings.value("volumeMute").toBool());
}

void MainWindow::tick(qint64 time) {
    if (time <= 0) {
        currentTime->clear();
        return;
    }

    currentTime->setText(formatTime(time));

    // remaining time
    const qint64 remainingTime = mediaObject->remainingTime();
    currentTime->setStatusTip(tr("Remaining time: %1").arg(formatTime(remainingTime)));

}

void MainWindow::totalTimeChanged(qint64 time) {
    if (time <= 0) {
        totalTime->clear();
        return;
    }
    totalTime->setText(formatTime(time));
}

QString MainWindow::formatTime(qint64 time) {
    QTime displayTime;
    displayTime = displayTime.addMSecs(time);
    QString timeString;
    // 60 * 60 * 1000 = 3600000
    if (time > 3600000)
        timeString = displayTime.toString("h:mm:ss");
    else
        timeString = displayTime.toString("m:ss");
    return timeString;
}

void MainWindow::volumeUp() {
    qreal newVolume = volumeSlider->audioOutput()->volume() + .1;
    if (newVolume > volumeSlider->maximumVolume())
        newVolume = volumeSlider->maximumVolume();
    volumeSlider->audioOutput()->setVolume(newVolume);
}

void MainWindow::volumeDown() {
    qreal newVolume = volumeSlider->audioOutput()->volume() - .1;
    if (newVolume < 0)
        newVolume = 0;
    volumeSlider->audioOutput()->setVolume(newVolume);
}

void MainWindow::volumeMute() {
    if (volumeSlider->audioOutput()) {
        volumeSlider->audioOutput()->setMuted(!volumeSlider->audioOutput()->isMuted());
    }
}

void MainWindow::volumeChanged(qreal newVolume) {
    if (volumeSlider->audioOutput())
        // automatically unmute when volume changes
        if (volumeSlider->audioOutput()->isMuted())
            volumeSlider->audioOutput()->setMuted(false);
    statusBar()->showMessage(tr("Volume at %1%").arg(newVolume*100));
}

void MainWindow::volumeMutedChanged(bool muted) {
    if (muted) {
        volumeMuteAct->setIcon(QtIconLoader::icon("audio-volume-muted"));
        statusBar()->showMessage(tr("Volume is muted"));
    } else {
        volumeMuteAct->setIcon(QtIconLoader::icon("audio-volume-high"));
        statusBar()->showMessage(tr("Volume is unmuted"));
    }
}

void MainWindow::setShuffle(bool enabled) {
    QSettings settings;
    settings.setValue("shuffle", QVariant::fromValue(enabled));
}

void MainWindow::setRepeat(bool enabled) {
    QSettings settings;
    settings.setValue("repeat", QVariant::fromValue(enabled));
}

void MainWindow::checkForUpdate() {
    static const QString updateCheckKey = "updateCheck";

    // check every 24h
    QSettings settings;
    uint unixTime = QDateTime::currentDateTime().toTime_t();
    int lastCheck = settings.value(updateCheckKey).toInt();
    int secondsSinceLastCheck = unixTime - lastCheck;
    // qDebug() << "secondsSinceLastCheck" << unixTime << lastCheck << secondsSinceLastCheck;
    if (secondsSinceLastCheck < 86400) return;

    // check it out
    if (updateChecker) delete updateChecker;
    updateChecker = new UpdateChecker();
    connect(updateChecker, SIGNAL(newVersion(QString)),
            this, SLOT(gotNewVersion(QString)));
    updateChecker->checkForUpdate();
    settings.setValue(updateCheckKey, unixTime);

}

void MainWindow::gotNewVersion(QString version) {
#ifdef APP_MAC_STORE
    return;
#endif

    QLabel *message = new QLabel(this);

    QString text = tr("%1 %2 is available!").arg(
                Constants::APP_NAME, version);

#if !defined(APP_MAC) && !defined(Q_WS_WIN)
    text += " " + tr("Please <a href='%2'>update now</a>.")
            .arg(QString(Constants::WEBSITE).append("#download"));
#endif
    message->setText(text);
    message->setOpenExternalLinks(true);
    if (updateChecker) delete updateChecker;
    statusBar()->addWidget(message);
}

QString MainWindow::playlistPath() {
    const QString storageLocation =
            QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // We need to use a default name, so why not the application one? (minitunes.pls sounds fine)
    return QString("%1/%2.pls").arg(storageLocation).arg(qApp->applicationName().toLower());
}

void MainWindow::savePlaylist() {
    const PlaylistModel* playlistModel = mediaView->getPlaylistModel();
    if (playlistModel == 0)
        return;

    QString plsPath = playlistPath();

    QFile plsFile(plsPath);
    QTextStream plsStream(&plsFile);
    if (!plsFile.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug() << "Cannot open file" << plsPath;
        return;
    }
    if (!playlistModel->saveTo(plsStream)) {
        qDebug() << "Error saving playlist";
    }
}

void MainWindow::loadPlaylist() {

    QString plsPath = playlistPath();
    if (!QFile::exists(plsPath))
        return;
    PlaylistModel* playlistModel = mediaView->getPlaylistModel();
    if (playlistModel == 0)
        return;
    // qDebug() << "Loading playlist: " << plsPath;
    QFile plsFile(plsPath);
    QTextStream plsStream(&plsFile);
    if ( plsFile.open(QFile::ReadOnly) )
        playlistModel->loadFrom(plsStream);
    else
        qDebug() << "Cannot open file" << plsPath;
}

void MainWindow::search(QString query) {
    showMediaView();
    mediaView->search(query);
    toolbarSearch->setFocus();
}

void MainWindow::searchCleared() {
    mediaView->search("");
}
