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
#include "constants.h"
#include "contextualview.h"
#include "database.h"
#include "iconutils.h"
#include "spacer.h"
#if defined(APP_MAC_SEARCHFIELD) && !defined(APP_MAC_QMACTOOLBAR)
#include "searchlineedit_mac.h"
#else
#include "searchlineedit.h"
#endif
#ifdef APP_MAC_QMACTOOLBAR
#include "mactoolbar.h"
#endif
#include "aboutview.h"
#include "choosefolderview.h"
#include "collectionscanner.h"
#include "collectionscannerview.h"
#include "fontutils.h"
#include "globalshortcuts.h"
#include "mediaview.h"
#include "updatechecker.h"
#include "view.h"
#ifdef Q_OS_MAC
#include "mac_startup.h"
#include "macfullscreen.h"
#include "macsupport.h"
#include "macutils.h"
#elif defined Q_OS_UNIX
#include "gnomeglobalshortcutbackend.h"
#endif
#include "collectionsuggester.h"
#include "imagedownloader.h"
#include "lastfm.h"
#include "lastfmlogindialog.h"
#include <iostream>
#include <utility>
#ifdef APP_EXTRA
#include "compositefader.h"
#include "extra.h"
#include "fader.h"
#include "updatedialog.h"
#endif
#include "http.h"
#include "httputils.h"
#include "seekslider.h"
#include "toolbarmenu.h"

#ifdef MEDIA_QTAV
#include "mediaqtav.h"
#endif
#ifdef MEDIA_MPV
#include "mediampv.h"
#endif

namespace {
MainWindow *singleton = nullptr;
}

MainWindow *MainWindow::instance() {
    if (!singleton) singleton = new MainWindow();
    return singleton;
}

MainWindow::MainWindow() : updateChecker(nullptr), toolbarMenu(nullptr), mainToolBar(nullptr) {
    fullScreenActive = false;

    singleton = this;

    setWindowTitle(Constants::NAME);

    // lazily initialized views
    mediaView = nullptr;
    collectionScannerView = nullptr;
    chooseFolderView = nullptr;
    aboutView = nullptr;
    contextualView = nullptr;

    // build ui
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();

    // views mechanism
    views = new QStackedWidget(this);
    setCentralWidget(views);

    // remove that useless menu/toolbar context menu
    setContextMenuPolicy(Qt::NoContextMenu);

#ifdef APP_EXTRA
    Extra::windowSetup(this);
#endif

    // restore window position
    readSettings();

    showInitialView();

    // event filter to block ugly toolbar tooltips
    qApp->installEventFilter(this);

    QTimer::singleShot(50, this, SLOT(lazyInit()));
}

void MainWindow::lazyInit() {
    GlobalShortcuts &shortcuts = GlobalShortcuts::instance();
#ifdef APP_MAC
    mac::MacSetup();
#elif defined Q_OS_UNIX
    if (GnomeGlobalShortcutBackend::IsGsdAvailable())
        shortcuts.setBackend(new GnomeGlobalShortcutBackend(&shortcuts));
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

        actionMap["finetune"]->setVisible(true);

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

bool MainWindow::eventFilter(QObject *obj, QEvent *e) {
    const QEvent::Type t = e->type();

#ifndef APP_MAC
    static bool altPressed = false;
    if (t == QEvent::KeyRelease && altPressed) {
        altPressed = false;
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Alt) {
            toggleMenuVisibility();
            return true;
        }
    } else if (t == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        altPressed = ke->key() == Qt::Key_Alt;
    }
#endif

    if (t == QEvent::ToolTip) {
        // kill tooltips
        return true;
    }

    if (t == QEvent::Show && obj == toolbarMenu) {
#ifdef APP_MAC
        int x = width() - toolbarMenu->sizeHint().width();
        int y = views->y();
#else
        int x = toolbarMenuButton->x() + toolbarMenuButton->width() -
                toolbarMenu->sizeHint().width();
        int y = toolbarMenuButton->y() + toolbarMenuButton->height();
#endif
        QPoint p(x, y);
        toolbarMenu->move(mapToGlobal(p));
    }

    if (t == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
        // qDebug() << keyEvent;
        if (keyEvent->key() == Qt::Key_MediaStop) {
            qDebug() << "Stop!";
            return false;
        }
    }

    if (t == QEvent::StyleChange) {
        qDebug() << "Style change detected";
        qApp->paletteChanged(qApp->palette());
        return false;
    }

    // standard event processing
    return QObject::eventFilter(obj, e);
}

void MainWindow::createActions() {
    backAct = new QAction(tr("&Back"), this);
    backAct->setEnabled(false);
    backAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    backAct->setStatusTip(tr("Go to the previous view"));
    actionMap.insert("back", backAct);
    connect(backAct, SIGNAL(triggered()), SLOT(goBack()));

    QIcon icon = IconUtils::icon(QStringList() << "audio-headphones"
                                               << "gtk-info"
                                               << "help-about");
    contextualAct = new QAction(icon, tr("&Info"), this);
    contextualAct->setStatusTip(tr("Show information about the current track"));
    contextualAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
    contextualAct->setEnabled(false);
    contextualAct->setCheckable(true);
    actionMap.insert("contextual", contextualAct);
    connect(contextualAct, SIGNAL(triggered()), SLOT(toggleContextualView()));

    /*
    stopAct = new QAction(
            IconUtils::icon("media-playback-stop"),
            tr("&Stop"), this);
    stopAct->setStatusTip(tr("Stop playback and go back to the search view"));
    stopAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::Key_Escape) <<
    QKeySequence(Qt::Key_MediaStop)); stopAct->setEnabled(false); actions->insert("stop", stopAct);
    connect(stopAct, SIGNAL(triggered()), SLOT(stop()));
    */

    skipBackwardAct = new QAction(tr("P&revious"), this);
    IconUtils::setIcon(skipBackwardAct, "media-skip-backward");
    skipBackwardAct->setStatusTip(tr("Go back to the previous track"));
    skipBackwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
#if QT_VERSION >= 0x040600
    skipBackwardAct->setPriority(QAction::LowPriority);
#endif
    skipBackwardAct->setEnabled(false);
    actionMap.insert("previous", skipBackwardAct);

    skipForwardAct = new QAction(tr("&Next"), this);
    IconUtils::setIcon(skipForwardAct, "media-skip-forward");
    skipForwardAct->setStatusTip(tr("Skip to the next track"));
    skipForwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
#if QT_VERSION >= 0x040600
    skipForwardAct->setPriority(QAction::LowPriority);
#endif
    skipForwardAct->setEnabled(false);
    actionMap.insert("skip", skipForwardAct);

    playAct = new QAction(tr("&Play"), this);
    IconUtils::setIcon(playAct, "media-playback-start");
    playAct->setStatusTip(tr("Start playback"));
    playAct->setShortcuts(QList<QKeySequence>()
                          << QKeySequence(Qt::Key_Space) << QKeySequence(Qt::Key_MediaPlay));
    playAct->setEnabled(false);
    playAct->setCheckable(true);
#ifdef APP_MAC
    playAct->setPriority(QAction::LowPriority);
#endif
    actionMap.insert("play", playAct);

    fullscreenAct = new QAction(tr("&Full Screen"), this);
    IconUtils::setIcon(fullscreenAct, "view-fullscreen");
    fullscreenAct->setStatusTip(tr("Go full screen"));
    QList<QKeySequence> fsShortcuts;
#ifdef APP_MAC
    fsShortcuts << QKeySequence(Qt::CTRL + Qt::META + Qt::Key_F);
#else
    fsShortcuts << QKeySequence(Qt::Key_F11) << QKeySequence(Qt::ALT + Qt::Key_Return);
#endif
    fullscreenAct->setShortcuts(fsShortcuts);
    fullscreenAct->setShortcutContext(Qt::ApplicationShortcut);
    actionMap.insert("fullscreen", fullscreenAct);
    connect(fullscreenAct, SIGNAL(triggered()), SLOT(toggleFullscreen()));

    removeAct = new QAction(tr("&Remove"), this);
    removeAct->setStatusTip(tr("Remove the selected tracks from the playlist"));
    removeAct->setShortcuts(QList<QKeySequence>()
                            << QKeySequence("Del") << QKeySequence("Backspace"));
    removeAct->setEnabled(false);
    actionMap.insert("remove", removeAct);

    moveUpAct = new QAction(tr("Move &Up"), this);
    moveUpAct->setStatusTip(tr("Move up the selected tracks in the playlist"));
    moveUpAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    moveUpAct->setEnabled(false);
    actionMap.insert("moveUp", moveUpAct);

    moveDownAct = new QAction(tr("Move &Down"), this);
    moveDownAct->setStatusTip(tr("Move down the selected tracks in the playlist"));
    moveDownAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    moveDownAct->setEnabled(false);
    actionMap.insert("moveDown", moveDownAct);

    quitAct = new QAction(tr("&Quit"), this);
    quitAct->setMenuRole(QAction::QuitRole);
    quitAct->setShortcut(QKeySequence(QKeySequence::Quit));
    quitAct->setStatusTip(tr("Bye"));
    actionMap.insert("quit", quitAct);
    connect(quitAct, SIGNAL(triggered()), SLOT(quit()));

    chooseFolderAct = new QAction(tr("&Change collection folder..."), this);
    chooseFolderAct->setStatusTip(tr("Choose a different music collection folder"));
    chooseFolderAct->setMenuRole(QAction::ApplicationSpecificRole);
    actionMap.insert("chooseFolder", chooseFolderAct);
    connect(chooseFolderAct, SIGNAL(triggered()), SLOT(showChooseFolderView()));

    siteAct = new QAction(tr("&Website"), this);
    siteAct->setShortcut(QKeySequence::HelpContents);
    siteAct->setStatusTip(tr("%1 on the Web").arg(Constants::NAME));
    actionMap.insert("site", siteAct);
    connect(siteAct, SIGNAL(triggered()), SLOT(visitSite()));

    donateAct = new QAction(tr("Make a &donation"), this);
    donateAct->setStatusTip(
            tr("Please support the continued development of %1").arg(Constants::NAME));
    actionMap.insert("donate", donateAct);
    connect(donateAct, SIGNAL(triggered()), SLOT(donate()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setMenuRole(QAction::AboutRole);
    aboutAct->setStatusTip(tr("Info about %1").arg(Constants::NAME));
    actionMap.insert("about", aboutAct);
    connect(aboutAct, SIGNAL(triggered()), SLOT(about()));

    // Anon
    QAction *action;

    action = new QAction(tr("&Fix Library with %1...").arg("Finetune"), this);
    action->setMenuRole(QAction::ApplicationSpecificRole);
    action->setVisible(false);
    actionMap.insert("finetune", action);
    connect(action, SIGNAL(triggered()), SLOT(runFinetune()));

    action = new QAction(tr("&Report an Issue..."), this);
    actionMap.insert("report-issue", action);
    connect(action, SIGNAL(triggered()), SLOT(reportIssue()));

    action = new QAction(tr("&Clear"), this);
    IconUtils::setIcon(action, "edit-clear");
    action->setShortcut(QKeySequence::New);
    action->setStatusTip(tr("Remove all tracks from the playlist"));
    action->setEnabled(false);
    actionMap.insert("clearPlaylist", action);

    action = new QAction(tr("&Shuffle"), this);
    IconUtils::setIcon(action, "media-playlist-shuffle");
    action->setStatusTip(tr("Random playlist mode"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), SLOT(setShuffle(bool)));
    actionMap.insert("shufflePlaylist", action);

    action = new QAction(tr("&Repeat"), this);
    IconUtils::setIcon(action, "media-playlist-repeat");
    action->setStatusTip(tr("Play first song again after all songs are played"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), SLOT(setRepeat(bool)));
    actionMap.insert("repeatPlaylist", action);

    action = new QAction(tr("&Close"), this);
    action->setShortcut(QKeySequence(QKeySequence::Close));
    actionMap.insert("close", action);
    connect(action, SIGNAL(triggered()), SLOT(close()));

    action = new QAction(Constants::NAME, this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_1));
    actionMap.insert("restore", action);
    connect(action, SIGNAL(triggered()), SLOT(restore()));

    action = new QAction(tr("&Stop After This Track"), this);
    action->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Escape));
    action->setCheckable(true);
    action->setEnabled(false);
    actionMap.insert("stopafterthis", action);
    connect(action, SIGNAL(toggled(bool)), SLOT(showStopAfterThisInStatusBar(bool)));

    action = new QAction(tr("&Scrobble"), this);
    IconUtils::setIcon(action, "audioscrobbler");
    action->setStatusTip(tr("Send played tracks titles to %1").arg("Last.fm"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    action->setCheckable(true);
    actionMap.insert("scrobbling", action);
    connect(action, SIGNAL(toggled(bool)), SLOT(toggleScrobbling(bool)));

    action = new QAction(tr("&Log Out from %1").arg("Last.fm"), this);
    action->setMenuRole(QAction::ApplicationSpecificRole);
    action->setEnabled(false);
    action->setVisible(false);
    actionMap.insert("lastFmLogout", action);
    connect(action, SIGNAL(triggered()), SLOT(lastFmLogout()));

    action = new QAction(tr("Toggle &Menu Bar"), this);
    connect(action, SIGNAL(triggered()), SLOT(toggleMenuVisibilityWithMessage()));
    actionMap.insert("toggleMenu", action);

    action = new QAction(tr("Menu"), this);
    IconUtils::setIcon(action, "open-menu");
    connect(action, SIGNAL(triggered()), SLOT(toggleToolbarMenu()));
    actionMap.insert("toolbarMenu", action);

#ifdef APP_MAC_STORE
    action = new QAction(tr("&Love %1? Rate it!").arg(Constants::NAME), this);
    actionMap.insert("app-store", action);
    connect(action, SIGNAL(triggered()), SLOT(rateOnAppStore()));
#endif

    // Invisible actions

    searchFocusAct = new QAction(this);
    searchFocusAct->setShortcut(QKeySequence::Find);
    searchFocusAct->setStatusTip(tr("Search"));
    actionMap.insert("search", searchFocusAct);
    connect(searchFocusAct, SIGNAL(triggered()), SLOT(searchFocus()));
    addAction(searchFocusAct);

    volumeUpAct = new QAction(this);
    volumeUpAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    actionMap.insert("volume-up", volumeUpAct);
    connect(volumeUpAct, SIGNAL(triggered()), SLOT(volumeUp()));
    addAction(volumeUpAct);

    volumeDownAct = new QAction(this);
    volumeDownAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    actionMap.insert("volume-down", volumeDownAct);
    connect(volumeDownAct, SIGNAL(triggered()), SLOT(volumeDown()));
    addAction(volumeDownAct);

    volumeMuteAct = new QAction(this);
    IconUtils::setIcon(volumeMuteAct, "audio-volume-high");
    volumeMuteAct->setStatusTip(tr("Mute volume"));
    volumeMuteAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    actionMap.insert("volume-mute", volumeMuteAct);
    connect(volumeMuteAct, SIGNAL(triggered()), SLOT(volumeMute()));
    addAction(volumeMuteAct);

    // common action properties
    for (QAction *action : qAsConst(actionMap)) {
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
            action->setStatusTip(action->statusTip() + QLatin1String(" (") +
                                 action->shortcut().toString(QKeySequence::NativeText) +
                                 QLatin1String(")"));
    }
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu(tr("&Application"));
    fileMenu->addAction(actionMap.value("finetune"));
    fileMenu->addAction(chooseFolderAct);
    fileMenu->addAction(actionMap.value("lastFmLogout"));
#ifndef APP_MAC
    fileMenu->addSeparator();
#else
    // fileMenu->addSeparator()->setMenuRole(QAction::ApplicationSpecificRole);
#endif
    fileMenu->addAction(quitAct);

    playbackMenu = menuBar()->addMenu(tr("&Playback"));
    menuMap.insert("playback", playbackMenu);
    // playbackMenu->addAction(stopAct);
    playbackMenu->addAction(playAct);
    playbackMenu->addAction(actionMap.value("stopafterthis"));
    playbackMenu->addSeparator();
    playbackMenu->addAction(skipForwardAct);
    playbackMenu->addAction(skipBackwardAct);
    playbackMenu->addSeparator();
    playbackMenu->addAction(actionMap.value("scrobbling"));
#ifdef APP_MAC
    MacSupport::dockMenu(playbackMenu);
#endif

    playlistMenu = menuBar()->addMenu(tr("Play&list"));
    menuMap.insert("playlist", playlistMenu);
    playlistMenu->addAction(actionMap.value("clearPlaylist"));
    playlistMenu->addSeparator();
    playlistMenu->addAction(actionMap.value("shufflePlaylist"));
    playlistMenu->addAction(actionMap.value("repeatPlaylist"));
    playlistMenu->addSeparator();
    playlistMenu->addAction(removeAct);
    playlistMenu->addAction(moveUpAct);
    playlistMenu->addAction(moveDownAct);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(contextualAct);
    viewMenu->addSeparator();
#ifndef APP_MAC
    viewMenu->addAction(fullscreenAct);
#endif

#ifdef APP_MAC
    MacSupport::windowMenu(this);
#endif

    helpMenu = menuBar()->addMenu(tr("&Help"));
    menuMap.insert("help", helpMenu);
    helpMenu->addAction(siteAct);
    helpMenu->addAction(donateAct);
    helpMenu->addAction(actionMap.value("report-issue"));
    helpMenu->addAction(aboutAct);

#ifdef APP_MAC_STORE
    helpMenu->addSeparator();
    helpMenu->addAction(actionMap.value("app-store"));
#endif
}

void MainWindow::createToolBar() {
    // Create widgets
    currentTimeLabel = new QLabel("00:00", this);

    seekSlider = new SeekSlider(this);
    seekSlider->setEnabled(false);
    seekSlider->setTracking(false);
    seekSlider->setMaximum(1000);

    volumeSlider = new SeekSlider(this);
    volumeSlider->setValue(volumeSlider->maximum());

#if defined(APP_MAC_SEARCHFIELD) && !defined(APP_MAC_QMACTOOLBAR)
    SearchWrapper *searchWrapper = new SearchWrapper(this);
    toolbarSearch = searchWrapper->getSearchLineEdit();
#else
    toolbarSearch = new SearchLineEdit(this);
#endif
    toolbarSearch->setMinimumWidth(toolbarSearch->fontInfo().pixelSize() * 15);
    toolbarSearch->setSuggester(new CollectionSuggester(this));
    connect(toolbarSearch, SIGNAL(search(const QString &)), SLOT(search(const QString &)));
    connect(toolbarSearch, SIGNAL(suggestionAccepted(Suggestion *)),
            SLOT(suggestionAccepted(Suggestion *)));
    toolbarSearch->setStatusTip(searchFocusAct->statusTip());

    // Add widgets to toolbar

#ifdef APP_MAC_QMACTOOLBAR
    currentTimeLabel->hide();
    toolbarSearch->hide();
    volumeSlider->hide();
    seekSlider->hide();
    MacToolbar::instance().createToolbar(this);
    return;
#endif

    mainToolBar = new QToolBar(this);
    addToolBar(mainToolBar);

    mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mainToolBar->setFloatable(false);
    mainToolBar->setMovable(false);
#ifndef APP_LINUX
    mainToolBar->setIconSize(QSize(32, 32));
#endif

    mainToolBar->addAction(skipBackwardAct);
    mainToolBar->addAction(playAct);
    mainToolBar->addAction(skipForwardAct);
    mainToolBar->addAction(contextualAct);

    mainToolBar->addWidget(new Spacer());

    currentTimeLabel->setFont(FontUtils::small());
    mainToolBar->addWidget(currentTimeLabel);

    mainToolBar->addWidget(new Spacer());

    seekSlider->setOrientation(Qt::Horizontal);
    seekSlider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    seekSlider->setFocusPolicy(Qt::NoFocus);
    mainToolBar->addWidget(seekSlider);

    mainToolBar->addWidget(new Spacer());

    mainToolBar->addAction(volumeMuteAct);
    QToolButton *volumeMuteButton =
            qobject_cast<QToolButton *>(mainToolBar->widgetForAction(volumeMuteAct));
    volumeMuteButton->setIconSize(QSize(16, 16));
    volumeMuteButton->connect(
            volumeMuteAct, &QAction::changed, volumeMuteButton,
            [volumeMuteButton] { volumeMuteButton->setIcon(volumeMuteButton->icon().pixmap(16)); });

    volumeSlider->setStatusTip(
            tr("Press %1 to raise the volume, %2 to lower it")
                    .arg(volumeUpAct->shortcut().toString(QKeySequence::NativeText),
                         volumeDownAct->shortcut().toString(QKeySequence::NativeText)));

    volumeSlider->setOrientation(Qt::Horizontal);
    // this makes the volume slider smaller
    volumeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    volumeSlider->setFocusPolicy(Qt::NoFocus);
    mainToolBar->addWidget(volumeSlider);

    mainToolBar->addWidget(new Spacer());

    mainToolBar->addWidget(toolbarSearch);
    Spacer *spacer = new Spacer();
    spacer->setWidth(4);
    mainToolBar->addWidget(spacer);

#ifndef APP_MAC
    QAction *toolbarMenuAction = getAction("toolbarMenu");
    mainToolBar->addAction(toolbarMenuAction);
    toolbarMenuButton =
            qobject_cast<QToolButton *>(mainToolBar->widgetForAction(toolbarMenuAction));
#endif
}

void MainWindow::createStatusBar() {
    statusToolBar = new QToolBar(this);
    statusToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    statusToolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    statusToolBar->setIconSize(QSize(16, 16));

    QWidget *spring = new QWidget();
    spring->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    statusToolBar->addWidget(spring);

    statusToolBar->addAction(actionMap.value("scrobbling"));
    statusToolBar->addAction(actionMap.value("shufflePlaylist"));
    statusToolBar->addAction(actionMap.value("repeatPlaylist"));
    statusToolBar->addAction(actionMap.value("clearPlaylist"));
    statusBar()->addPermanentWidget(statusToolBar);

    statusBar()->show();
}

void MainWindow::readSettings() {
    QSettings settings;
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    } else {
        const QRect desktopSize = qApp->desktop()->availableGeometry();
        int w = desktopSize.width() * .75;
        int h = desktopSize.height();
        setGeometry(
                QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, QSize(w, h), desktopSize));
    }
    maximizedBeforeFullScreen = isMaximized();
    actionMap.value("shufflePlaylist")->setChecked(settings.value("shuffle").toBool());
    actionMap.value("repeatPlaylist")->setChecked(settings.value("repeat").toBool());

    bool scrobbling = settings.value("scrobbling").toBool();
    actionMap.value("scrobbling")->setChecked(scrobbling);
    toggleScrobbling(scrobbling);

#ifndef APP_MAC
    menuBar()->setVisible(settings.value("menuBar", false).toBool());
#endif
}

QAction *MainWindow::getAction(const char *name) {
    return actionMap.value(QByteArray::fromRawData(name, strlen(name)));
}

void MainWindow::addNamedAction(const QByteArray &name, QAction *action) {
    actionMap.insert(name, action);
}

QMenu *MainWindow::getMenu(const char *name) {
    return menuMap.value(QByteArray::fromRawData(name, strlen(name)));
}

void MainWindow::writeSettings() {
    QSettings settings;

    // do not save geometry when in full screen
    if (!fullScreenActive) settings.setValue("geometry", saveGeometry());

    if (mediaView) {
        if (media->volume() > 0.1) settings.setValue("volume", media->volume());
        // settings.setValue("volumeMute", audioOutput->isMuted());
        mediaView->saveSplitterState();
    }

#ifndef APP_MAC
    settings.setValue("menuBar", menuBar()->isVisible());
#endif
}

void MainWindow::goBack() {
    if (history.size() > 1) {
        history.pop();
        QWidget *widget = history.pop();
        showWidget(widget);
    }
}

void MainWindow::showWidget(QWidget *widget, bool transition) {
#ifdef APP_MAC
    if (transition) CompositeFader::go(this, this->grab());
#endif

    setUpdatesEnabled(false);

    QWidget *oldWidget = views->currentWidget();
    View *oldView = qobject_cast<View *>(oldWidget);

    View *newView = qobject_cast<View *>(widget);
    if (newView) {
        newView->appear();
        QHash<QString, QVariant> metadata = newView->metadata();
        QString desc = metadata.value("description").toString();
        if (!desc.isEmpty()) showMessage(desc);
    }

    aboutAct->setEnabled(widget != aboutView);
    chooseFolderAct->setEnabled(widget == mediaView || widget == contextualView);
    toolbarSearch->setEnabled(widget == mediaView || widget == contextualView);
    if (mainToolBar) mainToolBar->setVisible(widget == mediaView || widget == contextualView);
    statusBar()->setVisible(widget == mediaView);

    views->setCurrentWidget(widget);

    setUpdatesEnabled(true);

    if (oldView) oldView->disappear();

    history.push(widget);

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
    QUrl url("https://flavio.tordini.org/donate/");
    statusBar()->showMessage(QString(tr("Opening %1").arg(url.toString())));
    QDesktopServices::openUrl(url);
}

void MainWindow::quit() {
    savePlaylist();
    writeSettings();
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *e) {
#ifdef APP_MAC
    mac::closeWindow(winId());
    e->ignore();
#else
    quit();
    QMainWindow::closeEvent(e);
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
        mediaView = new MediaView(this);
        connect(playAct, SIGNAL(triggered()), mediaView, SLOT(playPause()));
        views->addWidget(mediaView);
        QTimer::singleShot(0, this, &MainWindow::initMedia);
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

    bool isShown = views->currentWidget() == contextualView;
    if (isShown) {
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
            statusBar()->hide();
        } else
            contextualAct->setChecked(false);
    }
}

void MainWindow::hideContextualView() {
    goBack();
    contextualAct->setChecked(false);

    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(Qt::CTRL + Qt::Key_I);
    contextualAct->setShortcuts(shortcuts);

    statusBar()->show();
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
    scannerThread->setDirectory(std::move(directory));
    connect(scannerThread, SIGNAL(finished(QVariantMap)), SLOT(fullScanFinished(QVariantMap)),
            Qt::UniqueConnection);
    scannerThread->start();

    if (mediaView) {
        stop();
        PlaylistModel *playlistModel = mediaView->getPlaylistModel();
        if (playlistModel) {
            playlistModel->clear();
        }
    }
}

void MainWindow::fullScanFinished(const QVariantMap &stats) {
    emit collectionCreated();

    showMediaView();
    activateWindow();
#ifdef APP_EXTRA
    Extra::notify(tr("%1 finished scanning your music collection").arg(Constants::NAME), "", "");
#else
    QApplication::alert(this, 0);
#endif
#ifdef APP_WIN
    QApplication::alert(this, 0);
#endif
    startImageDownload();
    showFinetuneDialog(stats);
    actionMap["finetune"]->setVisible(true);
}

void MainWindow::startIncrementalScan() {
    showMessage(tr("Updating collection..."));
    chooseFolderAct->setEnabled(false);
    CollectionScannerThread *scannerThread = new CollectionScannerThread();
    // incremental!
    scannerThread->setDirectory(QString());
    connect(scannerThread, SIGNAL(progress(int)), SLOT(incrementalScanProgress(int)),
            Qt::UniqueConnection);
    connect(scannerThread, SIGNAL(finished(QVariantMap)),
            SLOT(incrementalScanFinished(QVariantMap)), Qt::UniqueConnection);
    scannerThread->start();
}

void MainWindow::incrementalScanProgress(int percent) {
    showMessage(tr("Updating collection - %1%").arg(QString::number(percent)));
}

void MainWindow::incrementalScanFinished(const QVariantMap &stats) {
    if (views->currentWidget() == mediaView || views->currentWidget() == contextualView)
        chooseFolderAct->setEnabled(true);
    showMessage(tr("Collection updated"));
    startImageDownload();
    showFinetuneDialog(stats);
}

void MainWindow::startImageDownload() {
    chooseFolderAct->setEnabled(false);
    ImageDownloaderThread *downloaderThread = new ImageDownloaderThread();
    connect(downloaderThread, SIGNAL(imageDownloaded()), SLOT(update()));
    connect(downloaderThread, SIGNAL(finished()), SLOT(imageDownloadFinished()));
    downloaderThread->start();
}

void MainWindow::imageDownloadFinished() {
    if (views->currentWidget() == mediaView || views->currentWidget() == contextualView)
        chooseFolderAct->setEnabled(true);
}

void MainWindow::stateChanged(Media::State state) {
    // play action
    if (state == Media::PlayingState) {
        playAct->setChecked(true);
    } else if (state == Media::StoppedState || state == Media::PausedState) {
        playAct->setChecked(false);
    }

    seekSlider->setEnabled(state != Media::StoppedState);

    switch (state) {
    case Media::ErrorState:
        showMessage(tr("Error: %1").arg(media->errorString()));
        break;

    case Media::BufferingState:
    case Media::LoadingState:
        currentTimeLabel->clear();
        break;

    default:;
    }
}

void MainWindow::stop() {
    media->stop();
    currentTimeLabel->clear();
}

void MainWindow::resizeEvent(QResizeEvent *e) {
    Q_UNUSED(e);
#ifdef APP_MAC
    if (mac::CanGoFullScreen(winId())) {
        bool isFullscreen = mac::IsFullScreen(winId());
        if (isFullscreen != fullScreenActive) {
            fullScreenActive = isFullscreen;
            updateUIForFullscreen();
        }
    }
#endif
#ifdef APP_MAC_QMACTOOLBAR
    toolbarSearch->move(width() - toolbarSearch->width() - 7, -38);
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

    fullScreenActive = !fullScreenActive;

    if (fullScreenActive) {
        // Enter fullscreen

        maximizedBeforeFullScreen = isMaximized();

        // save geometry now, if the user quits when in full screen
        // geometry won't be saved
        writeSettings();

#ifdef APP_MAC
        hide();
        views->setParent(nullptr);
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
        if (maximizedBeforeFullScreen)
            showMaximized();
        else
            showNormal();
#endif
        activateWindow();
    }

    updateUIForFullscreen();
}

void MainWindow::updateUIForFullscreen() {
    static QList<QKeySequence> fsShortcuts;
    static QString fsText;

    if (fullScreenActive) {
        fsShortcuts = fullscreenAct->shortcuts();
        fsText = fullscreenAct->text();
        fullscreenAct->setShortcuts(QList<QKeySequence>(fsShortcuts)
                                    << QKeySequence(Qt::Key_Escape));
        fullscreenAct->setText(tr("Leave &Full Screen"));
        fullscreenAct->setIcon(IconUtils::icon("view-restore"));

#ifndef APP_MAC
        menuVisibleBeforeFullScreen = menuBar()->isVisible();
        menuBar()->hide();
#endif

    } else {
        fullscreenAct->setShortcuts(fsShortcuts);
        fullscreenAct->setText(fsText);
        fullscreenAct->setIcon(IconUtils::icon("view-fullscreen"));

#ifndef APP_MAC
        menuBar()->setVisible(menuVisibleBeforeFullScreen);
#endif
    }

    statusBar()->setVisible(!fullScreenActive);

#ifndef APP_MAC
    if (fullScreenActive)
        mainToolBar->addAction(fullscreenAct);
    else
        mainToolBar->removeAction(fullscreenAct);
#endif
}

void MainWindow::searchFocus() {
    toolbarSearch->selectAll();
    toolbarSearch->setFocus();
}

void MainWindow::initMedia() {
#ifdef MEDIA_QTAV
    qFatal("QtAV has a showstopper bug. Audio stops randomly. See bug "
           "https://github.com/wang-bin/QtAV/issues/1184");
    media = new MediaQtAV(this);
#elif defined MEDIA_MPV
    media = new MediaMPV();
#else
    qFatal("No media backend defined");
#endif
    media->setAudioOnly(true);
    media->init();
    media->setBufferMilliseconds(10000);

    QSettings settings;
    volume = settings.value("volume", 1.).toReal();
    media->setVolume(volume);

    connect(media, &Media::error, this, &MainWindow::handleError);
    connect(media, &Media::stateChanged, this, &MainWindow::stateChanged);
    connect(media, &Media::positionChanged, this, &MainWindow::tick);

    connect(seekSlider, &QSlider::sliderMoved, this, [this](int value) {
        // value : maxValue = posit ion : duration
        qint64 ms = (value * media->duration()) / seekSlider->maximum();
        qDebug() << "Seeking to" << ms;
        media->seek(ms);
        if (media->state() == Media::PausedState) media->play();
    });
    connect(seekSlider, &QSlider::sliderPressed, this, [this]() {
        // value : maxValue = position : duration
        qint64 ms = (seekSlider->value() * media->duration()) / seekSlider->maximum();
        media->seek(ms);
        if (media->state() == Media::PausedState) media->play();
    });
    connect(media, &Media::started, this, [this]() { seekSlider->setValue(0); });

    connect(media, &Media::volumeChanged, this, &MainWindow::volumeChanged);
    connect(media, &Media::volumeMutedChanged, this, &MainWindow::volumeMutedChanged);
    connect(volumeSlider, &QSlider::sliderMoved, this, [this](int value) {
        qreal volume = (qreal)value / volumeSlider->maximum();
        media->setVolume(volume);
    });
    connect(volumeSlider, &QSlider::sliderPressed, this, [this]() {
        qreal volume = (qreal)volumeSlider->value() / volumeSlider->maximum();
        media->setVolume(volume);
    });

    mediaView->setMedia(media);
}

void MainWindow::tick(qint64 time) {
    // value : maxValue = position : duration
    qint64 duration = media->duration();
    if (duration <= 0) return;

    int value = (seekSlider->maximum() * media->position()) / duration;
    if (!seekSlider->isSliderDown()) seekSlider->setValue(value);

    const QString s = formatTime(time);
    if (s != currentTimeLabel->text()) {
        currentTimeLabel->setText(s);
        emit currentTimeChanged(s);

        // remaining time
        const qint64 remainingTime = media->remainingTime();
        currentTimeLabel->setStatusTip(tr("Remaining time: %1").arg(formatTime(remainingTime)));
    }
}

QString MainWindow::formatTime(qint64 duration) {
    duration /= 1000;
    QString res;
    int seconds = (int)(duration % 60);
    duration /= 60;
    int minutes = (int)(duration % 60);
    duration /= 60;
    int hours = (int)(duration % 24);
    if (hours == 0) return res.sprintf("%02d:%02d", minutes, seconds);
    return res.sprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

void MainWindow::volumeUp() {
    qreal newVolume = media->volume() + .1;
    if (newVolume > 1.) newVolume = 1.;
    media->setVolume(newVolume);
}

void MainWindow::volumeDown() {
    qreal newVolume = media->volume() - .1;
    if (newVolume < 0) newVolume = 0;
    media->setVolume(newVolume);
}

void MainWindow::volumeMute() {
    bool isMuted = media->volumeMuted();
    if (isMuted) {
        // unmuting
        media->setVolumeMuted(!isMuted);
        media->setVolume(volume);
    } else {
        // muting
        volume = media->volume();
        media->setVolumeMuted(!isMuted);
    }
}

void MainWindow::volumeChanged(qreal newVolume) {
    qDebug() << newVolume;
    // automatically unmute when volume changes
    if (media->volumeMuted()) media->setVolumeMuted(false);
    volume = media->volume();
    showMessage(tr("Volume at %1%").arg((int)(newVolume * 100)));
    // newVolume : 1.0 = x : 1000
    int value = newVolume * volumeSlider->maximum();
    volumeSlider->blockSignals(true);
    volumeSlider->setValue(value);
    volumeSlider->blockSignals(false);
}

void MainWindow::volumeMutedChanged(bool muted) {
    if (muted) {
        volumeMuteAct->setIcon(IconUtils::icon("audio-volume-muted"));
        statusBar()->showMessage(tr("Volume is muted"));
    } else {
        volumeMuteAct->setIcon(IconUtils::icon("audio-volume-high"));
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
    const QString updateCheckKey = "updateCheck";

    // check every 24h
    QSettings settings;
    uint unixTime = QDateTime::currentDateTimeUtc().toTime_t();
    int lastCheck = settings.value(updateCheckKey).toInt();
    int secondsSinceLastCheck = unixTime - lastCheck;
    // qDebug() << "secondsSinceLastCheck" << unixTime << lastCheck << secondsSinceLastCheck;
    if (secondsSinceLastCheck < 86400) return;

    // check it out
    if (updateChecker) delete updateChecker;
    updateChecker = new UpdateChecker();
    connect(updateChecker, SIGNAL(newVersion(QString)), SLOT(gotNewVersion(QString)));
    updateChecker->checkForUpdate();
    settings.setValue(updateCheckKey, unixTime);
}

void MainWindow::gotNewVersion(const QString &version) {
    if (updateChecker) {
        delete updateChecker;
        updateChecker = nullptr;
    }

    QSettings settings;
    QString checkedVersion = settings.value("checkedVersion").toString();
    if (checkedVersion == version) return;

#ifdef APP_SIMPLEUPDATE
    simpleUpdateDialog(version);
#elif defined(APP_EXTRA) && !defined(APP_MAC)
    UpdateDialog *dialog = new UpdateDialog(version, this);
    dialog->show();
#endif
}

void MainWindow::simpleUpdateDialog(const QString &version) {
    QMessageBox msgBox(this);
    msgBox.setIconPixmap(IconUtils::pixmap(":/images/64x64/app.png", devicePixelRatioF()));
    msgBox.setText(tr("%1 version %2 is now available.").arg(Constants::NAME, version));
    msgBox.setModal(true);
    msgBox.setWindowModality(Qt::WindowModal);
    msgBox.addButton(QMessageBox::Close);
    QPushButton *laterButton = msgBox.addButton(tr("Remind me later"), QMessageBox::RejectRole);
    QPushButton *updateButton = msgBox.addButton(tr("Update"), QMessageBox::AcceptRole);
    msgBox.exec();
    if (msgBox.clickedButton() != laterButton) {
        QSettings settings;
        settings.setValue("checkedVersion", version);
    }
    if (msgBox.clickedButton() == updateButton) visitSite();
}

void MainWindow::showFinetuneDialog(const QVariantMap &stats) {
    uint trackCount = stats.value("trackCount").toUInt();
    int tracksNeedingFixCount = stats.value("tracksNeedingFix").toStringList().size();

    if (trackCount <= 0 && tracksNeedingFixCount <= 0) return;

    int percent = (tracksNeedingFixCount * 100) / trackCount;
    if (percent <= 5) return;

    QString message =
            tr("%1 added %2 tracks to your music library."
               " %3 tracks (%4%) have incomplete tags.")
                    .arg(QLatin1String(Constants::NAME), QString::number(trackCount),
                         QString::number(tracksNeedingFixCount), QString::number(percent));

    QString infoText = tr("Do you want to fix them now with %1?").arg("Finetune");

    QMessageBox msgBox(this);
    msgBox.setIconPixmap(IconUtils::pixmap(":/images/64x64/finetune.png", devicePixelRatioF()));
    msgBox.setText(message);
    msgBox.setInformativeText(infoText);
    msgBox.setModal(true);
    msgBox.setWindowModality(Qt::WindowModal);
    msgBox.addButton(QMessageBox::Close);
    QPushButton *acceptButton = msgBox.addButton(tr("Fix my music"), QMessageBox::AcceptRole);
    msgBox.exec();
    if (msgBox.clickedButton() == acceptButton) runFinetune(stats);
}

void MainWindow::runFinetune() {
    const QString collectionRoot = Database::instance().collectionRoot();
    runFinetune(collectionRoot);
}

void MainWindow::runFinetune(const QVariantMap &stats) {
    const QStringList files = stats.value("trackPaths").toStringList();
    const QString filename = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                             QLatin1String("/finetune.txt");
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        qWarning() << "Error opening file for writing" << file.fileName();
    QTextStream stream(&file);
    for (const QString &s : files) {
        stream << s << endl;
    }
    stream.flush();
    file.close();
    runFinetune(filename);
}

void MainWindow::runFinetune(const QString &filename) {
#if defined APP_MAC || defined APP_WIN
    if (Extra::runFinetune(filename)) return;
#else
    QProcess process;
    if (QProcess::startDetached("finetune", QStringList(filename))) return;
#endif

    const QString baseUrl = QLatin1String("http://") + Constants::ORG_DOMAIN;

#ifdef APP_MAC_STORE
    QString pageUrl = baseUrl + QLatin1String("/finetune");
    QDesktopServices::openUrl(pageUrl);
    return;
#endif

#ifdef APP_EXTRA

    const QString filesUrl = baseUrl + QLatin1String("/files/");
    QString url = filesUrl + "finetune/finetune.";

#ifdef APP_MAC
    const QString ext = "dmg";
#elif defined APP_WIN
    const QString ext = "exe";
#else
    const QString ext = "deb";
#endif
    url += ext;

    QPixmap pixmap = IconUtils::pixmap(":/images/64x64/finetune.png", devicePixelRatioF());
    UpdateDialog *dialog = new UpdateDialog(&pixmap, "Finetune", QString(), url, this);
    dialog->downloadUpdate();
    dialog->show();

#else
    QString url = baseUrl + QLatin1String("/finetune");
    QDesktopServices::openUrl(url);
#endif
}

QString MainWindow::playlistPath() {
    const QString storageLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    // We need to use a default name, so why not the application one?
    return QString("%1/%2.pls").arg(storageLocation).arg(Constants::UNIX_NAME);
}

void MainWindow::savePlaylist() {
    if (!mediaView) return;

    const PlaylistModel *playlistModel = mediaView->getPlaylistModel();
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
    if (!QFile::exists(plsPath)) return;
    PlaylistModel *playlistModel = mediaView->getPlaylistModel();
    if (playlistModel == nullptr) return;
    QFile plsFile(plsPath);
    if (plsFile.open(QFile::ReadOnly)) {
        QTextStream plsStream(&plsFile);
        playlistModel->loadFrom(plsStream);
    } else
        qDebug() << "Cannot open file" << plsPath;
}

void MainWindow::toggleMenuVisibility() {
    bool show = !menuBar()->isVisible();
    menuBar()->setVisible(show);
}

void MainWindow::toggleMenuVisibilityWithMessage() {
    bool show = !menuBar()->isVisible();
    menuBar()->setVisible(show);
    if (!show) {
        QMessageBox msgBox(this);
        msgBox.setText(tr("You can still access the menu bar by pressing the ALT key"));
        msgBox.setModal(true);
        msgBox.setWindowModality(Qt::WindowModal);
        msgBox.exec();
    }
}

void MainWindow::toggleToolbarMenu() {
    if (!toolbarMenu) toolbarMenu = new ToolbarMenu(this);
    if (toolbarMenu->isVisible())
        toolbarMenu->hide();
    else
        toolbarMenu->show();
}

#ifdef APP_MAC_STORE
void MainWindow::rateOnAppStore() {
    QDesktopServices::openUrl(QUrl("macappstore://userpub.itunes.apple.com"
                                   "/WebObjects/MZUserPublishing.woa/wa/addUserReview"
                                   "?id=474190659&type=Purple+Software"));
}
#endif

void MainWindow::search(QString query) {
    showMediaView();
    mediaView->search(query);
    toolbarSearch->setFocus();
}

void MainWindow::suggestionAccepted(Suggestion *suggestion) {
    showMediaView();
    mediaView->search(suggestion->value);
}

void MainWindow::searchCleared() {
    mediaView->search("");
}

void MainWindow::showStopAfterThisInStatusBar(bool show) {
    QAction *action = actionMap.value("stopafterthis");
    showActionInStatusBar(action, show);
}

void MainWindow::showActionInStatusBar(QAction *action, bool show) {
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

void MainWindow::handleError(const QString &message) {
    qWarning() << message;
    showMessage(message);
}

void MainWindow::showMessage(const QString &message) {
    statusBar()->showMessage(message, 5000);
}

void MainWindow::toggleScrobbling(bool enable) {
    QSettings settings;
    settings.setValue("scrobbling", enable);

    const bool isAuthorized = LastFm::instance().isAuthorized();

    // need login?
    if (enable && !isAuthorized) {
        LastFmLoginDialog *dialog = new LastFmLoginDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(dialog, SIGNAL(accepted()), SLOT(enableScrobbling()));
        connect(dialog, SIGNAL(rejected()), SLOT(disableScrobbling()));
        QTimer::singleShot(0, dialog, SLOT(show()));
    }

    // enable logout
    if (isAuthorized) {
        QAction *action = actionMap.value("lastFmLogout");
        action->setEnabled(true);
        action->setVisible(true);
    }
}

void MainWindow::enableScrobbling() {
    QAction *action = actionMap.value("lastFmLogout");
    action->setEnabled(true);
    action->setVisible(true);
}

void MainWindow::disableScrobbling() {
    actionMap.value("scrobbling")->setChecked(false);
    toggleScrobbling(false);
}

void MainWindow::lastFmLogout() {
    LastFm::instance().logout();
    disableScrobbling();
    QAction *action = actionMap.value("lastFmLogout");
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

    if (message == "--toggle-playing" && playAct->isEnabled())
        playAct->trigger();
    else if (message == "--next" && skipForwardAct->isEnabled())
        skipForwardAct->trigger();
    else if (message == "--previous" && skipBackwardAct->isEnabled())
        skipBackwardAct->trigger();
    else if (message == QLatin1String("--stop-after-this")) {
        actionMap.value("stopafterthis")->toggle();
    } else if (message == "--info" && contextualAct->isEnabled())
        contextualAct->trigger();
    else
        MainWindow::printHelp();
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

void MainWindow::reportIssue() {
    QUrl url("https://flavio.tordini.org/forums/forum/musique-forums/musique-troubleshooting");
    QDesktopServices::openUrl(url);
}
