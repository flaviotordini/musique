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
#include "aboutview.h"
#include "actionbutton.h"
#include "choosefolderview.h"
#include "collectionscanner.h"
#include "collectionscannerview.h"
#include "collectionsuggester.h"
#include "constants.h"
#include "contextualview.h"
#include "database.h"
#include "fontutils.h"
#include "globalshortcuts.h"
#include "httputils.h"
#include "iconutils.h"
#include "imagedownloader.h"
#include "js.h"
#include "lastfm.h"
#include "lastfmlogindialog.h"
#include "mediaview.h"
#include "messagebar.h"
#include "seekslider.h"
#include "spacer.h"
#include "toolbarmenu.h"
#include "view.h"
#include "zoomableui.h"

#if defined(APP_MAC_SEARCHFIELD) && !defined(APP_MAC_QMACTOOLBAR)
#include "searchlineedit_mac.h"
#else
#include "searchlineedit.h"
#endif

#ifdef Q_OS_MAC
#include "mac_startup.h"
#include "macfullscreen.h"
#include "macsupport.h"
#include "macutils.h"
#elif defined Q_OS_UNIX
#include "gnomeglobalshortcutbackend.h"
#endif

#ifdef APP_EXTRA
#include "compositefader.h"
#include "extra.h"
#include "fader.h"
#include "updatedialog.h"
#endif

#ifdef APP_MAC_QMACTOOLBAR
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "mactoolbarutils.h"
#else
#include "mactoolbar_qt5.h"
#endif
#endif

#ifdef MEDIA_QTAV
#include "mediaqtav.h"
#endif

#ifdef MEDIA_MPV
#include "mediampv.h"
#endif

#ifdef MEDIA_QT
#include "mediaqt.h"
#endif

#ifdef UPDATER
#include "updater.h"
#endif


namespace {
MainWindow *singleton = nullptr;
}

MainWindow *MainWindow::instance() {
    return singleton;
}

MainWindow::MainWindow() : toolbarMenu(nullptr), toolbar(nullptr) {
    fullScreenActive = false;

    singleton = this;

    setWindowTitle(Constants::NAME);

    // lazily initialized views
    mediaView = nullptr;
    collectionScannerView = nullptr;
    chooseFolderView = nullptr;
    aboutView = nullptr;
    contextualView = nullptr;

    // views mechanism
    views = new QStackedWidget(this);
    setCentralWidget(views);

    zoomableUI = new ZoomableUI(*this);

    // build ui
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();

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
    JS::instance().getNamFactory().setRequestHeaders(
            {{"User-Agent", HttpUtils::stealthUserAgent()}});
    JS::instance().initialize(QUrl(QLatin1String(Constants::WEBSITE) + "-ws/bundle3.js"));

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

        bool finetuneMenuVisible = true;
        actionMap["finetune"]->setVisible(finetuneMenuVisible);

        // update the collection when idle
        QTimer::singleShot(500, this, SLOT(startIncrementalScan()));

        QTimer::singleShot(1500, this, [this] {
            if (!maybeShowUpdateNag()) maybeShowMessageBar();
        });

#if defined(UPDATER) && defined(QT_NO_DEBUG_OUTPUT)
        Updater::instance().checkWithoutUI();
#endif

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

    if (t == QEvent::Show && obj == toolbarMenu) {
        int x = width() - toolbarMenu->sizeHint().width();
        int y = views->y();
        if (toolBarArea(toolbar) == Qt::BottomToolBarArea)
            y += views->height() - toolbarMenu->sizeHint().height();
        toolbarMenu->move(mapToGlobal(QPoint(x, y)));
    }

    else if (t == QEvent::StyleChange && obj == this) {
        qDebug() << "Style change detected";
        qApp->paletteChanged(qApp->palette());
    }

    // standard event processing
    return QObject::eventFilter(obj, e);
}

void MainWindow::createActions() {
    backAct = new QAction(tr("&Back"), this);
    backAct->setEnabled(false);
    backAct->setShortcut(Qt::ALT | Qt::Key_Left);
    backAct->setStatusTip(tr("Go to the previous view"));
    addNamedAction("back", backAct);
    connect(backAct, &QAction::triggered, this, &MainWindow::goBack);

    QIcon icon = IconUtils::icon({"audio-headphones", "gtk-info", "help-about"});
    contextualAct = new QAction(icon, tr("&Info"), this);
    contextualAct->setStatusTip(tr("Show information about the current track"));
    contextualAct->setShortcut(Qt::CTRL | Qt::Key_I);
    contextualAct->setEnabled(false);
    contextualAct->setCheckable(true);
    addNamedAction("contextual", contextualAct);
    connect(contextualAct, &QAction::triggered, this, &MainWindow::toggleContextualView);

    skipBackwardAct = new QAction(tr("P&revious"), this);
    IconUtils::setIcon(skipBackwardAct, "media-skip-backward");
    skipBackwardAct->setStatusTip(tr("Go back to the previous track"));
    skipBackwardAct->setShortcut(Qt::CTRL | Qt::Key_Left);
    skipBackwardAct->setPriority(QAction::LowPriority);
    skipBackwardAct->setEnabled(false);
    addNamedAction("previous", skipBackwardAct);

    skipForwardAct = new QAction(tr("&Next"), this);
    IconUtils::setIcon(skipForwardAct, "media-skip-forward");
    skipForwardAct->setStatusTip(tr("Skip to the next track"));
    skipForwardAct->setShortcut(Qt::CTRL | Qt::Key_Right);
    skipForwardAct->setPriority(QAction::LowPriority);
    skipForwardAct->setEnabled(false);
    addNamedAction("skip", skipForwardAct);

    playAct = new QAction(tr("&Play"), this);
    IconUtils::setIcon(playAct, "media-playback-start");
    playAct->setStatusTip(tr("Start playback"));
    playAct->setShortcuts({Qt::Key_Space, Qt::Key_MediaPlay});
    playAct->setEnabled(false);
    playAct->setCheckable(true);
#ifdef APP_MAC
    playAct->setPriority(QAction::LowPriority);
#endif
    addNamedAction("play", playAct);

    fullscreenAct = new QAction(tr("&Full Screen"), this);
    IconUtils::setIcon(fullscreenAct, "view-fullscreen");
    fullscreenAct->setStatusTip(tr("Go full screen"));
    QList<QKeySequence> fsShortcuts;
#ifdef APP_MAC
    fsShortcuts << QKeySequence(Qt::CTRL | Qt::META | Qt::Key_F);
#else
    fsShortcuts << QKeySequence(Qt::Key_F11) << QKeySequence(Qt::ALT | Qt::Key_Return);
#endif
    fullscreenAct->setShortcuts(fsShortcuts);
    fullscreenAct->setShortcutContext(Qt::ApplicationShortcut);
    addNamedAction("fullscreen", fullscreenAct);
    connect(fullscreenAct, &QAction::triggered, this, &MainWindow::toggleFullscreen);

    removeAct = new QAction(tr("&Remove"), this);
    removeAct->setStatusTip(tr("Remove the selected tracks from the playlist"));
    removeAct->setShortcuts({Qt::Key_Backspace, Qt::Key_Delete});
    removeAct->setEnabled(false);
    addNamedAction("remove", removeAct);

    moveUpAct = new QAction(tr("Move &Up"), this);
    moveUpAct->setStatusTip(tr("Move up the selected tracks in the playlist"));
    moveUpAct->setShortcut(Qt::CTRL | Qt::Key_Up);
    moveUpAct->setEnabled(false);
    addNamedAction("moveUp", moveUpAct);

    moveDownAct = new QAction(tr("Move &Down"), this);
    moveDownAct->setStatusTip(tr("Move down the selected tracks in the playlist"));
    moveDownAct->setShortcut(Qt::CTRL | Qt::Key_Down);
    moveDownAct->setEnabled(false);
    addNamedAction("moveDown", moveDownAct);

    quitAct = new QAction(tr("&Quit"), this);
    quitAct->setMenuRole(QAction::QuitRole);
    quitAct->setShortcut(QKeySequence::Quit);
    quitAct->setStatusTip(tr("Bye"));
    addNamedAction("quit", quitAct);
    connect(quitAct, SIGNAL(triggered()), SLOT(quit()));

    chooseFolderAct = new QAction(tr("&Change collection folder..."), this);
    chooseFolderAct->setStatusTip(tr("Choose a different music collection folder"));
    chooseFolderAct->setMenuRole(QAction::ApplicationSpecificRole);
    addNamedAction("chooseFolder", chooseFolderAct);
    connect(chooseFolderAct, &QAction::triggered, this, &MainWindow::showChooseFolderView);

    siteAct = new QAction(tr("&Website"), this);
    siteAct->setShortcut(QKeySequence::HelpContents);
    siteAct->setStatusTip(tr("%1 on the Web").arg(Constants::NAME));
    addNamedAction("site", siteAct);
    connect(siteAct, &QAction::triggered, this, &MainWindow::visitSite);

    donateAct = new QAction(tr("Make a &donation"), this);
    donateAct->setStatusTip(
            tr("Please support the continued development of %1").arg(Constants::NAME));
    addNamedAction("donate", donateAct);
    connect(donateAct, &QAction::triggered, this, &MainWindow::donate);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setMenuRole(QAction::AboutRole);
    aboutAct->setStatusTip(tr("Info about %1").arg(Constants::NAME));
    addNamedAction("about", aboutAct);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

    // Anon
    QAction *a;

    a = new QAction(tr("Rewind %1 seconds").arg(10));
    a->setShortcuts({Qt::Key_Left, Qt::SHIFT + Qt::Key_Left});
    a->setShortcutContext(Qt::WidgetShortcut);
    connect(a, &QAction::triggered, this, [this] {
        qint64 position = media->position();
        position -= 10000;
        if (position < 0) position = 0;
        media->seek(position);
    });
    addNamedAction("seekBackward", a);

    a = new QAction(tr("Fast forward %1 seconds").arg(10));
    a->setShortcuts({Qt::Key_Right, Qt::SHIFT + Qt::Key_Right});
    a->setShortcutContext(Qt::WidgetShortcut);
    connect(a, &QAction::triggered, this, [this] {
        qint64 position = media->position();
        position += 10000;
        qint64 duration = media->duration();
        if (position > duration) position = duration;
        media->seek(position);
    });
    addNamedAction("seekForward", a);

    a = new QAction(tr("&Fix Library with %1...").arg("Finetune"), this);
    a->setMenuRole(QAction::ApplicationSpecificRole);
    a->setVisible(false);
    addNamedAction("finetune", a);
    connect(a, &QAction::triggered, this, [this] { runFinetune(); });

    a = new QAction(tr("&Report an Issue..."), this);
    addNamedAction("report-issue", a);
    connect(a, &QAction::triggered, this, &MainWindow::reportIssue);

    a = new QAction(tr("&Clear"), this);
    IconUtils::setIcon(a, "edit-clear");
    a->setShortcuts({Qt::CTRL + Qt::Key_Backspace, QKeySequence::New});
    a->setStatusTip(tr("Remove all tracks from the playlist"));
    a->setEnabled(false);
    addNamedAction("clearPlaylist", a);

    a = new QAction(tr("&Shuffle"), this);
    IconUtils::setIcon(a, "media-playlist-shuffle");
    a->setStatusTip(tr("Random playlist mode"));
    a->setShortcut(Qt::CTRL | Qt::Key_S);
    a->setCheckable(true);
    connect(a, &QAction::toggled, this, &MainWindow::setShuffle);
    addNamedAction("shufflePlaylist", a);

    a = new QAction(tr("&Repeat"), this);
    IconUtils::setIcon(a, "media-playlist-repeat");
    a->setStatusTip(tr("Play first song again after all songs are played"));
    a->setShortcut(Qt::CTRL | Qt::Key_R);
    a->setCheckable(true);
    connect(a, &QAction::toggled, this, &MainWindow::setRepeat);
    addNamedAction("repeatPlaylist", a);

    a = new QAction(tr("&Close"), this);
    a->setShortcut(QKeySequence::Close);
    addNamedAction("close", a);
    connect(a, &QAction::triggered, this, &QWidget::close);

    a = new QAction(Constants::NAME, this);
    a->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_1);
    addNamedAction("restore", a);
    connect(a, &QAction::triggered, this, &MainWindow::restore);

    a = new QAction(tr("&Stop After This Track"), this);
    a->setShortcut(Qt::SHIFT | Qt::Key_Escape);
    a->setCheckable(true);
    a->setEnabled(false);
    addNamedAction("stopafterthis", a);
    connect(a, &QAction::toggled, this, &MainWindow::showStopAfterThisInStatusBar);

    a = new QAction(tr("&Scrobble"), this);
    IconUtils::setIcon(a, "audioscrobbler");
    a->setStatusTip(tr("Send played tracks titles to %1").arg("Last.fm"));
    a->setShortcut(Qt::CTRL | Qt::Key_L);
    a->setCheckable(true);
    addNamedAction("scrobbling", a);
    connect(a, &QAction::toggled, this, &MainWindow::toggleScrobbling);

    a = new QAction(tr("&Log Out from %1").arg("Last.fm"), this);
    a->setMenuRole(QAction::ApplicationSpecificRole);
    a->setEnabled(false);
    a->setVisible(false);
    addNamedAction("lastFmLogout", a);
    connect(a, &QAction::triggered, this, &MainWindow::lastFmLogout);

    a = new QAction(tr("Toggle &Menu Bar"), this);
    connect(a, &QAction::triggered, this, &MainWindow::toggleMenuVisibilityWithMessage);
    addNamedAction("toggleMenu", a);

    a = new QAction(tr("Menu"), this);
    IconUtils::setIcon(a, "open-menu");
    connect(a, &QAction::triggered, this, &MainWindow::toggleToolbarMenu);
    addNamedAction("toolbarMenu", a);

#ifdef APP_MAC_STORE
    a = new QAction(tr("&Love %1? Rate it!").arg(Constants::NAME), this);
    addNamedAction("app-store", a);
    connect(a, SIGNAL(triggered()), SLOT(rateOnAppStore()));
#endif

    // Invisible actions

    searchFocusAct = new QAction(this);
    searchFocusAct->setShortcut(QKeySequence::Find);
    searchFocusAct->setStatusTip(tr("Search"));
    addNamedAction("search", searchFocusAct);
    connect(searchFocusAct, &QAction::triggered, this, &MainWindow::searchFocus);

    volumeUpAct = new QAction(this);
    volumeUpAct->setShortcut(Qt::CTRL | Qt::Key_Up);
    addNamedAction("volume-up", volumeUpAct);
    connect(volumeUpAct, &QAction::triggered, this, &MainWindow::volumeUp);

    volumeDownAct = new QAction(this);
    volumeDownAct->setShortcut(Qt::CTRL | Qt::Key_Down);
    addNamedAction("volume-down", volumeDownAct);
    connect(volumeDownAct, &QAction::triggered, this, &MainWindow::volumeDown);

    volumeMuteAct = new QAction(this);
    IconUtils::setIcon(volumeMuteAct, "audio-volume-high");
    volumeMuteAct->setStatusTip(tr("Mute volume"));
    volumeMuteAct->setShortcut(Qt::CTRL | Qt::Key_E);
    addNamedAction("volume-mute", volumeMuteAct);
    connect(volumeMuteAct, &QAction::triggered, this, &MainWindow::toggleVolumeMute);

    // common action properties
    for (QAction *a : qAsConst(actionMap)) {
        // add actions to the MainWindow so that they work
        // when the menu is hidden
        addAction(a);

        // never autorepeat.
        // unexperienced users tend to keep keys pressed for a "long" time
        a->setAutoRepeat(false);

        // make the actions work when in fullscreen
        a->setShortcutContext(Qt::ApplicationShortcut);

        // show keyboard shortcuts in the status bar
        if (!a->shortcut().isEmpty())
            a->setStatusTip(a->statusTip() + QLatin1String(" (") +
                            a->shortcut().toString(QKeySequence::NativeText) + QLatin1String(")"));
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
    viewMenu->addActions(zoomableUI->getActions());
#ifndef APP_MAC
    viewMenu->addAction(fullscreenAct);
#endif

#ifdef APP_MAC
    MacSupport::windowMenu(this);
#endif

    helpMenu = menuBar()->addMenu(tr("&Help"));
    menuMap.insert("help", helpMenu);
    helpMenu->addAction(siteAct);
    helpMenu->addAction(actionMap.value("report-issue"));
#ifndef APP_MAC_STORE
    helpMenu->addAction(donateAct);
#endif
    helpMenu->addAction(aboutAct);
#ifdef UPDATER
    helpMenu->addAction(Updater::instance().getAction());
#endif

#ifdef APP_MAC_STORE
    helpMenu->addSeparator();
    helpMenu->addAction(actionMap.value("app-store"));
#endif

    toolbarMenu = new ToolbarMenu(this);
}

void MainWindow::createToolBar() {
    // Create widgets
    currentTimeLabel = new QLabel("00:00", this);

    seekSlider = new SeekSlider(this);
    seekSlider->setProperty("knobless", true);
    seekSlider->setEnabled(false);
    seekSlider->setTracking(false);
    seekSlider->setMaximum(1000);

    volumeSlider = new SeekSlider(this);
    {
        auto p = volumeSlider->palette();
        p.setColor(QPalette::Highlight, p.color(QPalette::Button));
        volumeSlider->setPalette(p);
    }
    volumeSlider->setValue(volumeSlider->maximum());

#if defined(APP_MAC_SEARCHFIELD) && !defined(APP_MAC_QMACTOOLBAR)
    SearchWrapper *searchWrapper = new SearchWrapper(this);
    toolbarSearch = searchWrapper->getSearchLineEdit();
#else
    toolbarSearch = new SearchLineEdit(this);
#endif
    toolbarSearch->setMinimumWidth(toolbarSearch->fontInfo().pixelSize() * 15);
    toolbarSearch->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    toolbarSearch->setSuggester(new CollectionSuggester(this));
    connect(toolbarSearch, SIGNAL(search(const QString &)), SLOT(search(const QString &)));
    connect(toolbarSearch, SIGNAL(searchCleared()), SLOT(searchCleared()));
    connect(toolbarSearch, SIGNAL(suggestionAccepted(Suggestion *)),
            SLOT(suggestionAccepted(Suggestion *)));
    toolbarSearch->setStatusTip(searchFocusAct->statusTip());

    // Add widgets to toolbar

#ifdef APP_MAC_QMACTOOLBAR
    currentTimeLabel->hide();
    toolbarSearch->hide();
    volumeSlider->hide();
    seekSlider->hide();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    mac::createToolbar(this);
#else
    MacToolbar::instance().createToolbar(this);
#endif
    return;
#endif

    toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setFloatable(false);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(32, 32));
    addToolBar(toolbar);

    toolbar->addWidget(new Spacer());

    auto addSmallToolbutton = [this](const char *name) {
        auto button = new ActionButton();
        button->setAction(getAction(name));
        button->setIconSize(QSize(16, 16));
        button->setFlat(true);
        button->setFocusPolicy(Qt::NoFocus);
        toolbar->addWidget(button);
    };

    addSmallToolbutton("shufflePlaylist");
    toolbar->addAction(skipBackwardAct);
    toolbar->addAction(playAct);
    toolbar->addAction(skipForwardAct);
    addSmallToolbutton("repeatPlaylist");

    toolbar->addAction(contextualAct);

    toolbar->addWidget(new Spacer());

    currentTimeLabel->setFont(FontUtils::small());
    currentTimeLabel->setContentsMargins(0, 0, 10, 0);
    toolbar->addWidget(currentTimeLabel);

    seekSlider->setOrientation(Qt::Horizontal);
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    sp.setHorizontalStretch(2);
    seekSlider->setSizePolicy(sp);
    seekSlider->setMaximumWidth(500);
    seekSlider->setFocusPolicy(Qt::NoFocus);
    toolbar->addWidget(seekSlider);

    toolbar->addWidget(new Spacer());

    addSmallToolbutton("volume-mute");

    volumeSlider->setStatusTip(
            tr("Press %1 to raise the volume, %2 to lower it")
                    .arg(volumeUpAct->shortcut().toString(QKeySequence::NativeText),
                         volumeDownAct->shortcut().toString(QKeySequence::NativeText)));

    volumeSlider->setOrientation(Qt::Horizontal);
    volumeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    volumeSlider->setFocusPolicy(Qt::NoFocus);
    toolbar->addWidget(volumeSlider);

    toolbar->addWidget(new Spacer());

    toolbar->addWidget(toolbarSearch);

#ifndef APP_MAC
    addSmallToolbutton("toolbarMenu");
#endif
}

void MainWindow::createStatusBar() {
    statusToolBar = new QToolBar(this);
    statusToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    statusToolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    statusToolBar->setIconSize(QSize(16, 16));

    QWidget *spring = new QWidget();
    spring->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    statusToolBar->addWidget(spring);

    statusToolBar->addAction(actionMap.value("clearPlaylist"));
    statusBar()->addPermanentWidget(statusToolBar);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->show();
}

void MainWindow::readSettings() {
    QSettings settings;
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    } else {
        const QRect desktopSize = QGuiApplication::primaryScreen()->availableGeometry();
        int w = desktopSize.width() * .75;
        int h = desktopSize.height() * .75;
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

    settings.setValue("v", Constants::VERSION);
}

void MainWindow::goBack() {
    if (history.size() > 1) {
        history.pop();
        View *view = history.pop();
        showView(view);
    }
}

void MainWindow::showView(View *view, bool transition) {
    if (!history.isEmpty() && view == history.top()) {
        qDebug() << "Attempting to show same view" << view;
        return;
    }

#ifdef APP_MAC
    if (transition) CompositeFader::go(this, this->grab());
#endif

    View *oldView = qobject_cast<View *>(views->currentWidget());

    view->appear();
    QHash<QString, QVariant> metadata = view->metadata();
    QString desc = metadata.value("description").toString();
    if (!desc.isEmpty()) showMessage(desc);

    aboutAct->setEnabled(view != aboutView);
    chooseFolderAct->setEnabled(view == mediaView || view == contextualView);
    toolbarSearch->setEnabled(view == mediaView || view == contextualView);
    if (toolbar) toolbar->setVisible(view == mediaView || view == contextualView);
    statusBar()->setVisible(view == mediaView);

    views->setCurrentWidget(view);

    if (oldView) oldView->disappear();

    history.push(view);

#ifdef APP_MAC
    mac::uncloseWindow(window()->winId());
#endif
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
    showView(chooseFolderView, transition);
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
    showView(mediaView, transition);
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
            showView(contextualView);
            contextualAct->setChecked(true);

            QList<QKeySequence> shortcuts;
            shortcuts << contextualAct->shortcuts() << Qt::Key_Escape;
            contextualAct->setShortcuts(shortcuts);
            statusBar()->hide();
        } else
            contextualAct->setChecked(false);
    }
}

void MainWindow::hideContextualView() {
    goBack();
    contextualAct->setChecked(false);
    contextualAct->setShortcut(Qt::CTRL | Qt::Key_I);
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
    showView(collectionScannerView);

    CollectionScannerThread &scannerThread = CollectionScannerThread::instance();
    collectionScannerView->setCollectionScannerThread(&scannerThread);
    scannerThread.setDirectory(std::move(directory));
    connect(&scannerThread, SIGNAL(finished(QVariantMap)), SLOT(fullScanFinished(QVariantMap)),
            Qt::UniqueConnection);
    scannerThread.start();

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
    showFinetuneDialog(stats);

    if (views->currentWidget() == mediaView || views->currentWidget() == contextualView)
        chooseFolderAct->setEnabled(true);

    ImageDownloader::instance().start();
    CollectionScannerThread::instance().disconnect(this);
}

void MainWindow::startIncrementalScan() {
    showMessage(tr("Updating collection..."));
    chooseFolderAct->setEnabled(false);
    CollectionScannerThread &scannerThread = CollectionScannerThread::instance();
    // incremental!
    scannerThread.setDirectory(QString());
    connect(&scannerThread, SIGNAL(progress(int)), SLOT(incrementalScanProgress(int)),
            Qt::UniqueConnection);
    connect(&scannerThread, SIGNAL(finished(QVariantMap)),
            SLOT(incrementalScanFinished(QVariantMap)), Qt::UniqueConnection);
    scannerThread.start();
}

void MainWindow::incrementalScanProgress(int percent) {
    showMessage(tr("Updating collection - %1%").arg(QString::number(percent)));
}

void MainWindow::incrementalScanFinished(const QVariantMap &stats) {
    if (views->currentWidget() == mediaView || views->currentWidget() == contextualView)
        chooseFolderAct->setEnabled(true);
    showMessage(tr("Collection updated"));
    showFinetuneDialog(stats);
    ImageDownloader::instance().start();
    CollectionScannerThread::instance().disconnect(this);
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

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Left) {
        getAction("seekBackward")->trigger();
    } else if (e->key() == Qt::Key_Right) {
        getAction("seekForward")->trigger();
    }
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
        fullscreenAct->setShortcuts(QList<QKeySequence>(fsShortcuts) << Qt::Key_Escape);
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
        toolbar->addAction(fullscreenAct);
    else
        toolbar->removeAction(fullscreenAct);
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
#elif defined MEDIA_QT
    media = new MediaQt();
#else
    qFatal("No media backend defined");
#endif
    media->setAudioOnly(true);
    media->init();
    media->setBufferMilliseconds(10000);

    QSettings settings;
    qreal volume = settings.value("volume", 1.).toReal();
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
    if (hours == 0) return res.asprintf("%02d:%02d", minutes, seconds);
    return res.asprintf("%02d:%02d:%02d", hours, minutes, seconds);
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

void MainWindow::toggleVolumeMute() {
    bool muted = media->volumeMuted();
    media->setVolumeMuted(!muted);
}

void MainWindow::volumeChanged(qreal newVolume) {
    qDebug() << newVolume;
    // automatically unmute when volume changes
    if (media->volumeMuted()) media->setVolumeMuted(false);
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

bool MainWindow::maybeShowUpdateNag() {
    QSettings settings;
    QString lastRunVersion = settings.value("v").toString();
    if (!lastRunVersion.isEmpty() && lastRunVersion != QLatin1String(Constants::VERSION)) {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setIconPixmap(windowIcon().pixmap(64));
        msgBox->setText(tr("Thanks for updating %1 to version %2!")
                                .arg(Constants::NAME, Constants::VERSION));
        msgBox->setInformativeText(tr("If you enjoy %1, perhaps having installed it months or "
                                      "even years ago, please "
                                      "consider becoming one of the people willing to support "
                                      "something you enjoy.")
                                           .arg(Constants::NAME));
        msgBox->addButton(QMessageBox::Close);
        QPushButton *donateButton = msgBox->addButton(tr("Donate"), QMessageBox::AcceptRole);
        msgBox->setDefaultButton(donateButton);
        donateButton->setDefault(true);
        donateButton->setFocus();
        connect(msgBox, &QDialog::accepted, this, [this] { donate(); });
        msgBox->open();
        return true;
    }
    return false;
}

void MainWindow::showFinetuneDialog(const QVariantMap &stats) {
    uint trackCount = stats.value("trackCount").toUInt();
    int tracksNeedingFixCount = stats.value("tracksNeedingFix").toStringList().size();

    if (trackCount <= 0 && tracksNeedingFixCount <= 0) return;

    int percent = (tracksNeedingFixCount * 100) / trackCount;

    bool finetuneMenuVisible = true;
    actionMap["finetune"]->setVisible(finetuneMenuVisible);

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
        stream << s << Qt::endl;
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

#ifdef APP_MAC_STORE
    // QString pageUrl = baseUrl + QLatin1String("/finetune");
    QString pageUrl = "macappstore://userpub.itunes.apple.com"
                      "/WebObjects/MZUserPublishing.woa/wa/addUserReview"
                      "?id=1089865507&type=Purple+Software";
    QDesktopServices::openUrl(pageUrl);
    return;
#endif

    const QString baseUrl = QLatin1String("https://") + Constants::ORG_DOMAIN;

#if defined APP_EXTRA && !defined APP_WIN_STORE
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
    UpdateDialog *dialog = new UpdateDialog(pixmap, "Finetune", QString(), url, this);
    dialog->downloadUpdate();
    dialog->show();

#else
    QString url = baseUrl + QLatin1String("/finetune");
    QDesktopServices::openUrl(url);
#endif
}

QString MainWindow::playlistPath() {
    const QString storageLocation =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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

void MainWindow::maybeShowMessageBar() {
    messageBar = new MessageBar();
    auto dockWidget = new QDockWidget();
    dockWidget->setTitleBarWidget(new QWidget());
    dockWidget->setWidget(messageBar);
    dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dockWidget->setVisible(false);
    // connect(dockWidget, &QDockWidget::visibilityChanged, messageBar, &QWidget::setVisible);
    addDockWidget(Qt::TopDockWidgetArea, dockWidget);

    QSettings settings;
    QString key;

    bool showMessages = settings.contains("geometry");
#ifdef APP_ACTIVATION
    showMessages = Activation::instance().isActivated();
#endif

#if defined APP_MAC && !defined APP_MAC_STORE
    if (showMessages && !settings.contains(key = "sofa")) {
        QString msg = tr("Need a remote control for %1? Try %2!").arg(Constants::NAME).arg("Sofa");
        messageBar->setMessage(msg);
        disconnect(messageBar);
        connect(messageBar, &MessageBar::clicked, this, [key] {
            QString url = "https://" + QLatin1String(Constants::ORG_DOMAIN) + '/' + key;
            QDesktopServices::openUrl(url);
        });
        connect(messageBar, &MessageBar::closed, this, [key] {
            QSettings settings;
            settings.setValue(key, true);
        });
        dockWidget->show();
        showMessages = false;
    }
#endif

#ifdef UPDATER
    connect(&Updater::instance(), &Updater::statusChanged, this, [this, dockWidget](auto status) {
        if (status == Updater::Status::UpdateDownloaded) {
            QString msg = tr("An update is ready to be installed. Quit and install update.");
            messageBar->setMessage(msg);
            disconnect(messageBar);
            connect(messageBar, &MessageBar::clicked, this, [] { qApp->quit(); });
            dockWidget->show();
        }
    });
#endif
}
