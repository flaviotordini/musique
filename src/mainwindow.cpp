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

#include "mainwindow.h"
#include "spacer.h"
#include "constants.h"
#include "utils.h"
#include "global.h"
#include "database.h"
#include <QtSql>
#include "contextualview.h"
#ifdef APP_MAC
#include "searchlineedit_mac.h"
#else
#include "searchlineedit.h"
#endif
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
#ifdef Q_WS_MAC
#include "mac_startup.h"
#include "macfullscreen.h"
#include "macsupport.h"
#include "macutils.h"
#endif
#include "collectionsuggester.h"
#include "lastfm.h"
#include "lastfmlogindialog.h"
#include "imagedownloader.h"
#include <iostream>
#ifdef APP_EXTRA
#include "extra.h"
#include "updatedialog.h"
#endif
#ifdef APP_ACTIVATION
#include "activation.h"
#include "activationview.h"
#include "activationdialog.h"
#endif

static MainWindow *singleton = 0;

MainWindow* MainWindow::instance() {
    if (!singleton) singleton = new MainWindow();
    return singleton;
}

MainWindow::MainWindow() : updateChecker(0) {
    m_fullscreen = false;

    singleton = this;

    // lazily initialized views
    mediaView = 0;
    collectionScannerView = 0;
    chooseFolderView = 0;
    aboutView = 0;
    contextualView = 0;

    // build ui
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    // views mechanism
    history = new QStack<QWidget*>();
    views = new QStackedWidget(this);
    views->hide();
    setCentralWidget(views);

    // remove that useless menu/toolbar context menu
    setContextMenuPolicy(Qt::NoContextMenu);

#ifdef APP_EXTRA
    Extra::windowSetup(this);
#endif

    // restore window position
    readSettings();

    showInitialView();

#ifdef APP_ACTIVATION
    if (!Activation::instance().isActivated())
        showActivationView(false);
#endif

    views->show();

    // event filter to block ugly toolbar tooltips
    qApp->installEventFilter(this);

    qApp->processEvents();
    QTimer::singleShot(50, this, SLOT(lazyInit()));
}

MainWindow::~MainWindow() {
    delete history;
}

void MainWindow::lazyInit() {
    // Global shortcuts
    GlobalShortcuts &shortcuts = GlobalShortcuts::instance();
#ifdef Q_WS_X11
    if (GnomeGlobalShortcutBackend::IsGsdAvailable())
        shortcuts.setBackend(new GnomeGlobalShortcutBackend(&shortcuts));
#endif
#ifdef APP_MAC
    mac::MacSetup();
#endif
    connect(&shortcuts, SIGNAL(PlayPause()), playAct, SLOT(trigger()));
    connect(&shortcuts, SIGNAL(Stop()), this, SLOT(stop()));
}

void MainWindow::showInitialView() {
    // show the initial view
    Database &db = Database::instance();
    if (db.status() == ScanComplete) {

        showMediaView(false);
        QTimer::singleShot(0, this, SLOT(loadPlaylist()));

        // update the collection when idle
        QTimer::singleShot(500, this, SLOT(startIncrementalScan()));

        QTimer::singleShot(1000, this, SLOT(checkForUpdate()));

    } else {
        // no db, do the first scan dance
        QString root = db.needsUpdate();
        if (!root.isEmpty() && QDir().exists(root))
            startFullScan(root);
        else
            showChooseFolderView(false);
    }
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

    QHash<QString, QAction*> *actions = The::globalActions();

    backAct = new QAction(tr("&Back"), this);
    backAct->setEnabled(false);
    backAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    backAct->setStatusTip(tr("Go to the previous view"));
    actions->insert("back", backAct);
    connect(backAct, SIGNAL(triggered()), SLOT(goBack()));

    QIcon icon = Utils::icon("gtk-info");
#ifdef Q_WS_X11
    if (icon.isNull()) {
        icon = Utils::icon("help-about");
    }
#endif
    contextualAct = new QAction(icon, tr("&Info"), this);
    contextualAct->setStatusTip(tr("Show information about the current track"));
    contextualAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    contextualAct->setEnabled(false);
    contextualAct->setCheckable(true);
    actions->insert("contextual", contextualAct);
    connect(contextualAct, SIGNAL(triggered()), SLOT(toggleContextualView()));

    /*
    stopAct = new QAction(
            Utils::icon("media-playback-stop"),
            tr("&Stop"), this);
    stopAct->setStatusTip(tr("Stop playback and go back to the search view"));
    stopAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::Key_Escape) << QKeySequence(Qt::Key_MediaStop));
    stopAct->setEnabled(false);
    actions->insert("stop", stopAct);
    connect(stopAct, SIGNAL(triggered()), SLOT(stop()));
    */

    skipBackwardAct = new QAction(
                Utils::icon("media-skip-backward"),
                tr("P&revious"), this);
    skipBackwardAct->setStatusTip(tr("Go back to the previous track"));
    skipBackwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
#if QT_VERSION >= 0x040600
    skipBackwardAct->setPriority(QAction::LowPriority);
#endif
    skipBackwardAct->setEnabled(false);
    actions->insert("previous", skipBackwardAct);

    skipForwardAct = new QAction(
                Utils::icon("media-skip-forward"),
                tr("&Next"), this);
    skipForwardAct->setStatusTip(tr("Skip to the next track"));
    skipForwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
#if QT_VERSION >= 0x040600
    skipForwardAct->setPriority(QAction::LowPriority);
#endif
    skipForwardAct->setEnabled(false);
    actions->insert("skip", skipForwardAct);

    playAct = new QAction(
                Utils::icon("media-playback-start"),
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

    fullscreenAct = new QAction(
                Utils::icon("view-restore"),
                tr("&Full Screen"), this);
    fullscreenAct->setStatusTip(tr("Go full screen"));
    QList<QKeySequence> fsShortcuts;
#ifdef APP_MAC
    fsShortcuts << QKeySequence(Qt::CTRL + Qt::META + Qt::Key_F);
#else
    fsShortcuts << QKeySequence(Qt::Key_F11) << QKeySequence(Qt::ALT + Qt::Key_Return);
#endif
    fullscreenAct->setShortcuts(fsShortcuts);
    fullscreenAct->setShortcutContext(Qt::ApplicationShortcut);
    actions->insert("fullscreen", fullscreenAct);
    connect(fullscreenAct, SIGNAL(triggered()), SLOT(toggleFullscreen()));

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
    quitAct->setShortcut(QKeySequence(QKeySequence::Quit));
    quitAct->setStatusTip(tr("Bye"));
    actions->insert("quit", quitAct);
    connect(quitAct, SIGNAL(triggered()), SLOT(quit()));

    chooseFolderAct = new QAction(tr("&Change collection folder..."), this);
    chooseFolderAct->setStatusTip(tr("Choose a different music collection folder"));
    chooseFolderAct->setMenuRole(QAction::ApplicationSpecificRole);
    actions->insert("chooseFolder", chooseFolderAct);
    connect(chooseFolderAct, SIGNAL(triggered()), SLOT(showChooseFolderView()));

    siteAct = new QAction(tr("&Website"), this);
    siteAct->setShortcut(QKeySequence::HelpContents);
    siteAct->setStatusTip(tr("%1 on the Web").arg(Constants::NAME));
    actions->insert("site", siteAct);
    connect(siteAct, SIGNAL(triggered()), SLOT(visitSite()));

    donateAct = new QAction(tr("Make a &donation"), this);
    donateAct->setStatusTip(tr("Please support the continued development of %1").arg(Constants::NAME));
    actions->insert("donate", donateAct);
    connect(donateAct, SIGNAL(triggered()), SLOT(donate()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setMenuRole(QAction::AboutRole);
    aboutAct->setStatusTip(tr("Info about %1").arg(Constants::NAME));
    actions->insert("about", aboutAct);
    connect(aboutAct, SIGNAL(triggered()), SLOT(about()));

    // Anon
    QAction *action;

    action = new QAction(tr("&Report an Issue..."), this);
    actions->insert("report-issue", action);
    connect(action, SIGNAL(triggered()), SLOT(reportIssue()));

    action = new QAction(Utils::icon("edit-clear"), tr("&Clear"), this);
    action->setShortcut(QKeySequence::New);
    action->setStatusTip(tr("Remove all tracks from the playlist"));
    action->setEnabled(false);
    actions->insert("clearPlaylist", action);

    action = new QAction(
                Utils::icon("media-playlist-shuffle"), tr("&Shuffle"), this);
    action->setStatusTip(tr("Random playlist mode"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), SLOT(setShuffle(bool)));
    actions->insert("shufflePlaylist", action);

    action = new QAction(
                Utils::icon("media-playlist-repeat"),
                tr("&Repeat"), this);
    action->setStatusTip(tr("Play first song again after all songs are played"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), SLOT(setRepeat(bool)));
    actions->insert("repeatPlaylist", action);

    action = new QAction(tr("&Close"), this);
    action->setShortcut(QKeySequence(QKeySequence::Close));
    actions->insert("close", action);
    connect(action, SIGNAL(triggered()), SLOT(close()));

    action = new QAction(Constants::NAME, this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_1));
    actions->insert("restore", action);
    connect(action, SIGNAL(triggered()), SLOT(restore()));

    action = new QAction(Utils::icon("media-playback-stop"),
                         tr("&Stop After This Track"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Escape));
    action->setCheckable(true);
    action->setEnabled(false);
    actions->insert("stopafterthis", action);
    connect(action, SIGNAL(toggled(bool)), SLOT(showStopAfterThisInStatusBar(bool)));

    action = new QAction(QIcon(":/images/audioscrobbler.png"), tr("&Scrobbling"), this);
    action->setStatusTip(tr("Send played tracks titles to %1").arg("Last.fm"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    action->setCheckable(true);
    actions->insert("scrobbling", action);
    connect(action, SIGNAL(toggled(bool)), SLOT(toggleScrobbling(bool)));

    action = new QAction(tr("&Log Out from %1").arg("Last.fm"), this);
    action->setMenuRole(QAction::ApplicationSpecificRole);
    action->setEnabled(false);
    action->setVisible(false);
    actions->insert("lastFmLogout", action);
    connect(action, SIGNAL(triggered()), SLOT(lastFmLogout()));

    // Invisible actions

    searchFocusAct = new QAction(this);
    searchFocusAct->setShortcut(QKeySequence::Find);
    searchFocusAct->setStatusTip(tr("Search"));
    actions->insert("search", searchFocusAct);
    connect(searchFocusAct, SIGNAL(triggered()), SLOT(searchFocus()));
    addAction(searchFocusAct);

    volumeUpAct = new QAction(this);
    volumeUpAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_Plus));
    actions->insert("volume-up", volumeUpAct);
    connect(volumeUpAct, SIGNAL(triggered()), SLOT(volumeUp()));
    addAction(volumeUpAct);

    volumeDownAct = new QAction(this);
    volumeDownAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_Minus));
    actions->insert("volume-down", volumeDownAct);
    connect(volumeDownAct, SIGNAL(triggered()), SLOT(volumeDown()));
    addAction(volumeDownAct);

    volumeMuteAct = new QAction(this);
    volumeMuteAct->setIcon(Utils::icon("audio-volume-high"));
    volumeMuteAct->setStatusTip(tr("Mute volume"));
    volumeMuteAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_E));
    actions->insert("volume-mute", volumeMuteAct);
    connect(volumeMuteAct, SIGNAL(triggered()), SLOT(volumeMute()));
    addAction(volumeMuteAct);

#ifdef APP_ACTIVATION
    Extra::createActivationAction(tr("Buy %1...").arg(Constants::NAME));
#endif

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
    }

}

void MainWindow::createMenus() {

    QHash<QString, QMenu*> *menus = The::globalMenus();

    fileMenu = menuBar()->addMenu(tr("&Application"));
#ifdef APP_ACTIVATION
    QAction *buyAction = The::globalActions()->value("buy");
    if (buyAction) fileMenu->addAction(buyAction);
#ifndef APP_MAC
    fileMenu->addSeparator();
#endif
#endif
    fileMenu->addAction(chooseFolderAct);
    fileMenu->addAction(The::globalActions()->value("lastFmLogout"));
#ifndef APP_MAC
    fileMenu->addSeparator();
#endif
    fileMenu->addAction(quitAct);

    playbackMenu = menuBar()->addMenu(tr("&Playback"));
    menus->insert("playback", playbackMenu);
    // playbackMenu->addAction(stopAct);
    playbackMenu->addAction(playAct);
    playbackMenu->addAction(The::globalActions()->value("stopafterthis"));
    playbackMenu->addSeparator();
    playbackMenu->addAction(skipForwardAct);
    playbackMenu->addAction(skipBackwardAct);
    playbackMenu->addSeparator();
    playbackMenu->addAction(The::globalActions()->value("scrobbling"));
#ifdef APP_MAC
    MacSupport::dockMenu(playbackMenu);
#endif

    playlistMenu = menuBar()->addMenu(tr("Play&list"));
    menus->insert("playlist", playlistMenu);
    playlistMenu->addAction(The::globalActions()->value("clearPlaylist"));
    playlistMenu->addSeparator();
    playlistMenu->addAction(The::globalActions()->value("shufflePlaylist"));
    playlistMenu->addAction(The::globalActions()->value("repeatPlaylist"));
    playlistMenu->addSeparator();
    playlistMenu->addAction(removeAct);
    playlistMenu->addAction(moveUpAct);
    playlistMenu->addAction(moveDownAct);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(contextualAct);
    viewMenu->addSeparator();
    viewMenu->addAction(fullscreenAct);

#ifdef APP_MAC
    MacSupport::windowMenu(this);
#endif

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(siteAct);
#if !defined(APP_MAC) && !defined(APP_WIN)
    helpMenu->addAction(donateAct);
#endif
    helpMenu->addAction(The::globalActions()->value("report-issue"));
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars() {

    setUnifiedTitleAndToolBarOnMac(true);
    mainToolBar = new QToolBar(this);
    mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
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

    QFont smallerFont = FontUtils::smaller();
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

#ifdef APP_MAC
    SearchWrapper* searchWrapper = new SearchWrapper(this);
    toolbarSearch = searchWrapper->getSearchLineEdit();
#else
    toolbarSearch = new SearchLineEdit(this);
#endif
    toolbarSearch->setMinimumWidth(toolbarSearch->fontInfo().pixelSize()*15);
    toolbarSearch->setSuggester(new CollectionSuggester(this));
    connect(toolbarSearch, SIGNAL(search(const QString&)), SLOT(search(const QString&)));
    connect(toolbarSearch, SIGNAL(cleared()), SLOT(searchCleared()));
    connect(toolbarSearch, SIGNAL(suggestionAccepted(const QString&)), SLOT(search(const QString&)));
    toolbarSearch->setStatusTip(searchFocusAct->statusTip());
#ifdef APP_MAC
    mainToolBar->addWidget(searchWrapper);
#else
    mainToolBar->addWidget(toolbarSearch);
    Spacer* spacer = new Spacer();
    // spacer->setWidth(4);
    mainToolBar->addWidget(spacer);
#endif

    addToolBar(mainToolBar);
}

void MainWindow::createStatusBar() {
    statusToolBar = new QToolBar(this);
    statusToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    statusToolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
#ifdef Q_WS_X11
    int iconHeight = 16;
    int iconWidth = iconHeight;
#else
    int iconHeight = 17;
    int iconWidth = iconHeight * 3 / 2;
#endif
    statusToolBar->setIconSize(QSize(iconWidth, iconHeight));

    QWidget *spring = new QWidget();
    spring->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    statusToolBar->addWidget(spring);

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

    bool scrobbling = settings.value("scrobbling").toBool();
    The::globalActions()->value("scrobbling")->setChecked(scrobbling);
    toggleScrobbling(scrobbling);
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
        showWidget(widget);
    }
}

void MainWindow::showWidget(QWidget* widget, bool transition) {

    setUpdatesEnabled(false);

    View* oldView = dynamic_cast<View *> (views->currentWidget());
    if (oldView) oldView->disappear();

    View* newView = dynamic_cast<View *> (widget);
    if (newView) {
        newView->appear();
        QHash<QString,QVariant> metadata = newView->metadata();
        QString title = metadata.value("title").toString();
        if (title.isEmpty()) title = Constants::NAME;
        else title += QLatin1String(" - ") + Constants::NAME;
        setWindowTitle(title);
        QString desc = metadata.value("description").toString();
        if (!desc.isEmpty()) showMessage(desc);
    }

    aboutAct->setEnabled(widget != aboutView);
    chooseFolderAct->setEnabled(widget == mediaView || widget == contextualView);
    toolbarSearch->setEnabled(widget == mediaView || widget == contextualView);

    QWidget *oldWidget = views->currentWidget();
    views->setCurrentWidget(widget);

    setUpdatesEnabled(true);

#ifndef Q_WS_X11
    if (transition)
        Extra::fadeInWidget(oldWidget, widget);
#endif

    history->push(widget);

#ifdef APP_MAC
        mac::uncloseWindow(window()->winId());
#endif

}

void MainWindow::about() {
    if (!aboutView) {
        aboutView = new AboutView(this);
        views->addWidget(aboutView);
    }
    showWidget(aboutView);
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
#ifdef APP_MAC
    mac::closeWindow(winId());
    event->ignore();
#else
    quit();
    QMainWindow::closeEvent(event);
#endif
}

void MainWindow::showChooseFolderView(bool transition) {
    if (!chooseFolderView) {
        chooseFolderView = new ChooseFolderView(this);
        connect(chooseFolderView, SIGNAL(locationChanged(QString)), SLOT(startFullScan(QString)));
        views->addWidget(chooseFolderView);
    }
    showWidget(chooseFolderView, transition);
}

void MainWindow::showMediaView(bool transition) {
    if (!mediaView) {
        initPhonon();
        mediaView = new MediaView(this);
        mediaView->hide();
        mediaView->setMediaObject(mediaObject);
        connect(playAct, SIGNAL(triggered()), mediaView, SLOT(playPause()));
        views->addWidget(mediaView);
    }

    if (views->currentWidget() == contextualView) {
        hideContextualView();
    }

    mediaView->setFocus();
    showWidget(mediaView, transition);
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
            showWidget(contextualView);
            contextualAct->setChecked(true);

            QList<QKeySequence> shortcuts;
            shortcuts << contextualAct->shortcuts() << QKeySequence(Qt::Key_Escape);
            contextualAct->setShortcuts(shortcuts);
        } else contextualAct->setChecked(false);
    }
}

void MainWindow::hideContextualView() {
    goBack();
    contextualAct->setChecked(false);

    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(Qt::CTRL + Qt::Key_I);
    contextualAct->setShortcuts(shortcuts);
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
    showWidget(collectionScannerView);

    CollectionScannerThread *scannerThread = new CollectionScannerThread();
    collectionScannerView->setCollectionScannerThread(scannerThread);
    scannerThread->setDirectory(directory);
    connect(scannerThread, SIGNAL(finished()), SLOT(fullScanFinished()), Qt::UniqueConnection);
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
#ifdef APP_EXTRA
    Extra::notify(tr("%1 finished scanning your music collection").arg(Constants::NAME), "", "");
#else
    QApplication::alert(this, 0);
#endif
#ifdef APP_WIN
    QApplication::alert(this, 0);
#endif
    startImageDownload();
}

void MainWindow::startIncrementalScan() {
    showMessage(tr("Updating collection..."));
    chooseFolderAct->setEnabled(false);
    CollectionScannerThread *scannerThread = new CollectionScannerThread();
    // incremental!
    scannerThread->setDirectory(QString());
    connect(scannerThread, SIGNAL(progress(int)), SLOT(incrementalScanProgress(int)), Qt::UniqueConnection);
    connect(scannerThread, SIGNAL(finished()), SLOT(incrementalScanFinished()), Qt::UniqueConnection);
    scannerThread->start();
}

void MainWindow::incrementalScanProgress(int percent) {
    showMessage(tr("Updating collection - %1%").arg(QString::number(percent)));
}

void MainWindow::incrementalScanFinished() {
    if (views->currentWidget() == mediaView ||
            views->currentWidget() == contextualView)
        chooseFolderAct->setEnabled(true);
    showMessage(tr("Collection updated"));
    startImageDownload();
}

void MainWindow::startImageDownload() {
    chooseFolderAct->setEnabled(false);
    ImageDownloaderThread *downloaderThread = new ImageDownloaderThread();
    connect(downloaderThread, SIGNAL(finished()), SLOT(imageDownloadFinished()));
    downloaderThread->start();
}

void MainWindow::imageDownloadFinished() {
    if (views->currentWidget() == mediaView ||
            views->currentWidget() == contextualView)
    chooseFolderAct->setEnabled(true);
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
            showMessage(tr("Fatal error: %1").arg(mediaObject->errorString()));
        } else {
            showMessage(tr("Error: %1").arg(mediaObject->errorString()));
        }
        break;

    case Phonon::PlayingState:
        // stopAct->setEnabled(true);
        break;

    case Phonon::StoppedState:
        // stopAct->setEnabled(false);
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
    }
}

void MainWindow::stop() {
    mediaObject->stop();
    currentTime->clear();
    totalTime->clear();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
#if APP_MAC
    if (mac::CanGoFullScreen(winId())) {
        bool isFullscreen = mac::IsFullScreen(winId());
        if (isFullscreen != m_fullscreen) {
            m_fullscreen = isFullscreen;
            updateUIForFullscreen();
        }
    }
#endif
}

void MainWindow::toggleFullscreen() {

#ifdef APP_MAC
    WId handle = winId();
    if (mac::CanGoFullScreen(handle)) {
        mac::ToggleFullScreen(handle);
        return;
    }
#endif

    m_fullscreen = !m_fullscreen;

    if (m_fullscreen) {
        // Enter fullscreen

        m_maximized = isMaximized();

        // save geometry now, if the user quits when in full screen
        // geometry won't be saved
        writeSettings();

#ifdef APP_MAC
        hide();
        views->setParent(0);
        QTimer::singleShot(0, views, SLOT(showFullScreen()));
#else
        showFullScreen();
#endif

    } else {
        // Exit fullscreen

#if APP_MAC
        setCentralWidget(views);
        views->showNormal();
        show();
#else
        if (m_maximized) showMaximized();
        else showNormal();
#endif
        activateWindow();

    }

    updateUIForFullscreen();

}

void MainWindow::updateUIForFullscreen() {
    static QList<QKeySequence> fsShortcuts;
    static QString fsText;

    if (m_fullscreen) {
        fsShortcuts = fullscreenAct->shortcuts();
        fsText = fullscreenAct->text();
        fullscreenAct->setShortcuts(QList<QKeySequence>(fsShortcuts)
                                    << QKeySequence(Qt::Key_Escape));
        fullscreenAct->setText(tr("Leave &Full Screen"));
    } else {
        fullscreenAct->setShortcuts(fsShortcuts);
        fullscreenAct->setText(fsText);
    }

#ifndef APP_MAC
    menuBar()->setVisible(!m_fullscreen);
#endif
    statusBar()->setVisible(!m_fullscreen);

#ifndef APP_MAC
    if (m_fullscreen)
        mainToolBar->addAction(fullscreenAct);
    else
        mainToolBar->removeAction(fullscreenAct);
#endif
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
        // the "if" is important because tick is continually called
        // and we don't want to paint the toolbar every 100ms
        if (!currentTime->text().isEmpty()) currentTime->clear();
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
        volumeMuteAct->setIcon(Utils::icon("audio-volume-muted"));
        statusBar()->showMessage(tr("Volume is muted"));
    } else {
        volumeMuteAct->setIcon(Utils::icon("audio-volume-high"));
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
    if (updateChecker) {
        delete updateChecker;
        updateChecker = 0;
    }

    QSettings settings;
    QString checkedVersion = settings.value("checkedVersion").toString();
    if (checkedVersion == version) return;

#ifdef APP_SIMPLEUPDATE
    simpleUpdateDialog(version);
#elif defined(APP_ACTIVATION) && !defined(APP_MAC)
    UpdateDialog *dialog = new UpdateDialog(version, this);
    dialog->show();
#endif
}

void MainWindow::simpleUpdateDialog(QString version) {
    QMessageBox msgBox(this);
    msgBox.setIconPixmap(
                QPixmap(":/images/app.png")
                .scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setText(tr("%1 version %2 is now available.").arg(Constants::NAME, version));
    msgBox.setModal(true);
    msgBox.setWindowModality(Qt::WindowModal);
    msgBox.addButton(QMessageBox::Close);
    QPushButton* laterButton = msgBox.addButton(tr("Remind me later"), QMessageBox::RejectRole);
    QPushButton* updateButton = msgBox.addButton(tr("Update"), QMessageBox::AcceptRole);
    msgBox.exec();
    if (msgBox.clickedButton() != laterButton) {
        QSettings settings;
        settings.setValue("checkedVersion", version);
    }
    if (msgBox.clickedButton() == updateButton) visitSite();
}

QString MainWindow::playlistPath() {
    const QString storageLocation =
            QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    // We need to use a default name, so why not the application one?
    return QString("%1/%2.pls").arg(storageLocation).arg(Constants::UNIX_NAME);
}

void MainWindow::savePlaylist() {
    if (!mediaView) return;

    const PlaylistModel* playlistModel = mediaView->getPlaylistModel();
    if (!playlistModel) return;

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

void MainWindow::showStopAfterThisInStatusBar(bool show) {
    QAction* action = The::globalActions()->value("stopafterthis");
    showActionInStatusBar(action, show);
}

void MainWindow::showActionInStatusBar(QAction* action, bool show) {
#ifdef APP_EXTRA
    Extra::fadeInWidget(statusBar(), statusBar());
#endif
    if (show) {
        if (!statusToolBar->actions().contains(action))
            // statusToolBar->insertAction(statusToolBar->actions().first(), action);
            statusToolBar->insertAction(statusToolBar->actions().at(1), action);
    } else {
        statusToolBar->removeAction(action);
    }
}

void MainWindow::handleError(QString message) {
    qWarning() << message;
    showMessage(message);
}

void MainWindow::showMessage(QString message) {
    statusBar()->showMessage(message, 5000);
}

void MainWindow::toggleScrobbling(bool enable) {
    QSettings settings;
    settings.setValue("scrobbling", enable);

    // show in status bar
    const bool isAuthorized = LastFm::instance().isAuthorized();
    const bool showInStatusBar = enable || isAuthorized;
    showActionInStatusBar(The::globalActions()->value("scrobbling"), showInStatusBar);

    // need login?
    if (enable && !isAuthorized) {
        LastFmLoginDialog* dialog = new LastFmLoginDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, SIGNAL(accepted()), SLOT(enableScrobbling()));
        connect(dialog, SIGNAL(rejected()), SLOT(disableScrobbling()));
        QTimer::singleShot(0, dialog, SLOT(show()));
    }

    // enable logout
    if (isAuthorized) {
        QAction* action = The::globalActions()->value("lastFmLogout");
        action->setEnabled(true);
        action->setVisible(true);
    }

}

void MainWindow::enableScrobbling() {
    QAction* action = The::globalActions()->value("lastFmLogout");
    action->setEnabled(true);
    action->setVisible(true);
}

void MainWindow::disableScrobbling() {
    The::globalActions()->value("scrobbling")->setChecked(false);
    toggleScrobbling(false);
}

void MainWindow::lastFmLogout() {
    LastFm::instance().logout();
    disableScrobbling();
    QAction* action = The::globalActions()->value("lastFmLogout");
    action->setEnabled(false);
    action->setVisible(false);
}

void MainWindow::restore() {
#ifdef APP_MAC
    mac::uncloseWindow(window()->winId());
#endif
}

void MainWindow::messageReceived(const QString &message) {
    if (message.isEmpty()) return;

    if (message == "--toggle-playing" && playAct->isEnabled()) playAct->trigger();
    else if (message == "--next" && skipForwardAct->isEnabled()) skipForwardAct->trigger();
    else if (message == "--previous" && skipBackwardAct->isEnabled()) skipBackwardAct->trigger();
    else if (message == "--info" && contextualAct->isEnabled()) contextualAct->trigger();
    else MainWindow::printHelp();
}

void MainWindow::printHelp() {
    QString msg = QString("%1 %2\n\n").arg(Constants::NAME, Constants::VERSION);
    msg += "Usage: musique [options]\n";
    msg += "Options:\n";
    msg += "  --toggle-playing\t";
    msg += "Start or pause playback.\n";
    msg += "  --next\t\t";
    msg += "Skip to the next track.\n";
    msg += "  --previous\t\t";
    msg += "Go back to the previous track.\n";
    msg += "  --info\t\t";
    msg += "Display information about the current track.\n";
    std::cout << msg.toLocal8Bit().data();
}

#ifdef APP_ACTIVATION
void MainWindow::showActivationView(bool transition) {
    QWidget *activationView = ActivationView::instance();
    if (views->currentWidget() == activationView) {
        buy();
        return;
    }
    views->addWidget(activationView);
    showWidget(activationView, transition);
}

void MainWindow::showActivationDialog() {
    QTimer::singleShot(0, new ActivationDialog(this), SLOT(show()));
}

void MainWindow::buy() {
    Extra::buy();
}

void MainWindow::hideBuyAction() {
    QAction *action = The::globalActions()->value("buy");
    action->setVisible(false);
    action->setEnabled(false);
}

void MainWindow::showDemoDialog(QString message) {
    QMessageBox msgBox(this);
    msgBox.setIconPixmap(QPixmap(":/images/app.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setText(message);
    msgBox.setModal(true);
    // make it a "sheet" on the Mac
    msgBox.setWindowModality(Qt::WindowModal);

    msgBox.addButton(QMessageBox::Ok);
    QPushButton *buyButton = msgBox.addButton(tr("Get the full version"), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == buyButton) {
        showActivationView();
    }
}
#endif

void MainWindow::reportIssue() {
    QUrl url("http://flavio.tordini.org/forums/forum/musique-forums/musique-troubleshooting");
    QDesktopServices::openUrl(url);
}
