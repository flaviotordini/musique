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
#include "collectionscannerview.h"

MainWindow::MainWindow() {
    m_fullscreen = false;

    // lazily initialized views
    collectionScannerView = 0;
    chooseFolderView = 0;
    aboutView = 0;
    contextualView = 0;

    toolbarSearch = new SearchLineEdit(this);
    toolbarSearch->setFont(qApp->font());
    toolbarSearch->setMinimumWidth(toolbarSearch->fontInfo().pixelSize()*15);
    // TODO connect(toolbarSearch, SIGNAL(search(const QString&)), searchView, SLOT(watch(const QString&)));

    initPhonon();

    // build ui
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    // views mechanism
    history = new QStack<QWidget*>();
    views = new QStackedWidget(this);

    // views
    mediaView = new MediaView(this);
    mediaView->setMediaObject(mediaObject);
    views->addWidget(mediaView);

    // remove that useless menu/toolbar context menu
    this->setContextMenuPolicy(Qt::NoContextMenu);

    setCentralWidget(views);

    // show the initial view
    Database &db = Database::instance();
    if (db.status() == ScanComplete) {

        showWidget(mediaView);

        // update the collection when idle
        QTimer::singleShot(1000, this, SLOT(startIncrementalScan()));

    } else {
        // no db, do the first scan dance
        showChooseFolderView();
    }

    // event filter to block ugly toolbar tooltips
    qApp->installEventFilter(this);

    // restore window position
    readSettings();
}

MainWindow::~MainWindow() {
    delete &Database::instance();
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

    QList<QKeySequence> shortcuts;

    settingsAct = new QAction(tr("&Preferences..."), this);
    settingsAct->setStatusTip(tr(QString("Configure ").append(Constants::APP_NAME).toUtf8()));
    // Mac integration
    settingsAct->setMenuRole(QAction::PreferencesRole);
    actions->insert("settings", settingsAct);
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(showSettings()));

    backAct = new QAction(tr("&Back"), this);
    backAct->setEnabled(false);
    backAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    backAct->setStatusTip(tr("Go to the previous view"));
    actions->insert("back", backAct);
    connect(backAct, SIGNAL(triggered()), this, SLOT(goBack()));

    contextualAct = new QAction(QtIconLoader::icon("help-about", QIcon(":/images/help-about.png")), tr("&Info"), this);
    contextualAct->setStatusTip(tr("Show information about the current track"));
    contextualAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space));
    contextualAct->setEnabled(false);
    contextualAct->setCheckable(true);
    actions->insert("contextual", contextualAct);
    connect(contextualAct, SIGNAL(triggered()), this, SLOT(toggleContextualView()));

    stopAct = new QAction(
            QtIconLoader::icon("media-playback-stop", QIcon(":/images/media-playback-stop.png")),
            tr("&Stop"), this);
    stopAct->setStatusTip(tr("Stop playback and go back to the search view"));
    stopAct->setShortcut(QKeySequence(Qt::Key_Escape));
    stopAct->setEnabled(false);
    actions->insert("stop", stopAct);
    connect(stopAct, SIGNAL(triggered()), this, SLOT(stop()));

    skipBackwardAct = new QAction(
            QtIconLoader::icon("media-skip-backward", QIcon(":/images/media-skip-backward.png")),
            tr("P&revious"), this);
    skipBackwardAct->setStatusTip(tr("Go back to the previous track"));
    skipBackwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
#if QT_VERSION >= 0x040600
    skipBackwardAct->setPriority(QAction::LowPriority);
#endif
    actions->insert("previous", skipBackwardAct);

    skipForwardAct = new QAction(
            QtIconLoader::icon("media-skip-forward", QIcon(":/images/media-skip-forward.png")),
            tr("&Next"), this);
    skipForwardAct->setStatusTip(tr("Skip to the next track"));
    skipForwardAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
#if QT_VERSION >= 0x040600
    skipForwardAct->setPriority(QAction::LowPriority);
#endif
    actions->insert("skip", skipForwardAct);

    playAct = new QAction(
            QtIconLoader::icon("media-playback-start", QIcon(":/images/media-playback-start.png")),
            tr("&Play"), this);
    playAct->setStatusTip(tr("Start playback"));
    shortcuts << QKeySequence(Qt::Key_Space) << QKeySequence(Qt::Key_MediaPlay);
    playAct->setShortcuts(shortcuts);
    playAct->setEnabled(true);
    actions->insert("play", playAct);
    connect(playAct, SIGNAL(triggered()), this, SLOT(playPause()));

    fullscreenAct = new QAction(
            QtIconLoader::icon("view-fullscreen", QIcon(":/images/view-fullscreen.png")),
            tr("&Full Screen"), this);
    fullscreenAct->setStatusTip(tr("Go full screen"));
    fullscreenAct->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Return));
    fullscreenAct->setShortcutContext(Qt::ApplicationShortcut);
    actions->insert("fullscreen", fullscreenAct);
    connect(fullscreenAct, SIGNAL(triggered()), this, SLOT(fullscreen()));

    removeAct = new QAction(tr("&Remove"), this);
    removeAct->setStatusTip(tr("Remove the selected tracks from the playlist"));
    shortcuts << QKeySequence("Del") << QKeySequence("Backspace");
    removeAct->setShortcuts(shortcuts);
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
    connect(chooseFolderAct, SIGNAL(triggered()), this, SLOT(showChooseFolderView()));

    siteAct = new QAction(tr("&Website"), this);
    siteAct->setShortcut(QKeySequence::HelpContents);
    siteAct->setStatusTip(tr("%1 on the Web").arg(Constants::APP_NAME));
    actions->insert("site", siteAct);
    connect(siteAct, SIGNAL(triggered()), this, SLOT(visitSite()));

    donateAct = new QAction(tr("Make a &donation"), this);
    donateAct->setStatusTip(tr("Please support the continued development of %1").arg(Constants::APP_NAME));
    actions->insert("donate", donateAct);
    connect(donateAct, SIGNAL(triggered()), this, SLOT(donate()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setMenuRole(QAction::AboutRole);
    aboutAct->setStatusTip(tr("Info about %1").arg(Constants::APP_NAME));
    actions->insert("about", aboutAct);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    // Anon
    QAction *action;

    action = new QAction(QtIconLoader::icon("edit-clear", QIcon(":/images/edit-clear.png")), tr("&Clear"), this);
    action->setShortcut(QKeySequence::New);
    action->setStatusTip(tr("Remove all tracks from the playlist"));
    action->setEnabled(false);
    actions->insert("clearPlaylist", action);

    action = new QAction(
            QtIconLoader::icon("media-playlist-shuffle", QIcon(":/images/media-playlist-shuffle.png")),
            tr("&Shuffle"), this);
    action->setStatusTip(tr("Random playlist mode"));
    action->setCheckable(true);
    actions->insert("shufflePlaylist", action);

    action = new QAction(
            QtIconLoader::icon("media-playlist-repeat", QIcon(":/images/media-playlist-repeat.png")),
            tr("&Repeat"), this);
    action->setStatusTip(tr("Play first song again after all songs are played"));
    action->setCheckable(true);
    actions->insert("repeatPlaylist", action);

    // Invisible actions

    searchFocusAct = new QAction(this);
    searchFocusAct->setShortcut(QKeySequence::Find);
    actions->insert("search", searchFocusAct);
    connect(searchFocusAct, SIGNAL(triggered()), this, SLOT(searchFocus()));
    addAction(searchFocusAct);

    volumeUpAct = new QAction(this);
    volumeUpAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    actions->insert("volume-up", volumeUpAct);
    connect(volumeUpAct, SIGNAL(triggered()), this, SLOT(volumeUp()));
    addAction(volumeUpAct);

    volumeDownAct = new QAction(this);
    volumeDownAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    actions->insert("volume-down", volumeDownAct);
    connect(volumeDownAct, SIGNAL(triggered()), this, SLOT(volumeDown()));
    addAction(volumeDownAct);

    volumeMuteAct = new QAction(this);
    volumeMuteAct->setShortcut(tr("Ctrl+M"));
    actions->insert("volume-mute", volumeMuteAct);
    connect(volumeMuteAct, SIGNAL(triggered()), this, SLOT(volumeMute()));
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

#ifndef Q_WS_MAC
    fileMenu = menuBar()->addMenu(tr("&Application"));
    fileMenu->addAction(chooseFolderAct);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAct);
#endif

    playbackMenu = menuBar()->addMenu(tr("&Playback"));
    menus->insert("playback", playbackMenu);
    playbackMenu->addAction(stopAct);
    playbackMenu->addAction(playAct);
    playbackMenu->addSeparator();
    playbackMenu->addAction(skipForwardAct);
    playbackMenu->addAction(skipBackwardAct);

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
    viewMenu->addSeparator();
    viewMenu->addAction(fullscreenAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(siteAct);
    helpMenu->addAction(donateAct);
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars() {

    setUnifiedTitleAndToolBarOnMac(true);
    mainToolBar = new QToolBar(this);
#if QT_VERSION < 0x040600 || defined(Q_WS_MAC)
    mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#else
    mainToolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#endif
    mainToolBar->setFloatable(false);
    mainToolBar->setMovable(false);

    QFont smallerFont;
    smallerFont.setPointSize(smallerFont.pointSize()*.85);
    // mainToolBar->setFont(smallerFont);

    mainToolBar->addAction(playAct);
    mainToolBar->addAction(skipBackwardAct);
    mainToolBar->addAction(skipForwardAct);
    mainToolBar->addAction(contextualAct);

    mainToolBar->addWidget(new Spacer(mainToolBar, new QWidget(this)));

    currentTime = new QLabel(mainToolBar);
    currentTime->setFont(smallerFont);
    mainToolBar->addWidget(currentTime);

    seekSlider = new Phonon::SeekSlider(this);
    seekSlider->setMediaObject(mediaObject);
    seekSlider->setIconVisible(false);
    Spacer *seekSliderSpacer = new Spacer(mainToolBar, seekSlider);
    seekSliderSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainToolBar->addWidget(seekSliderSpacer);

    totalTime = new QLabel(mainToolBar);
    totalTime->setFont(smallerFont);
    mainToolBar->addWidget(totalTime);

    mainToolBar->addWidget(new Spacer(mainToolBar, new QWidget(this)));

    volumeSlider = new Phonon::VolumeSlider(this);
    volumeSlider->setAudioOutput(audioOutput);
    // this makes the volume slider smaller
    volumeSlider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mainToolBar->addWidget(new Spacer(mainToolBar, volumeSlider));

    mainToolBar->addWidget(new Spacer(mainToolBar, new QWidget(this)));

    mainToolBar->addWidget(new Spacer(mainToolBar, toolbarSearch));

    addToolBar(mainToolBar);
}

void MainWindow::createStatusBar() {

    // remove ugly borders on OSX
    statusBar()->setStyleSheet(
            "::item{border:0 solid} QStatusBar, QToolBar {padding:0;margin:0} QToolButton {padding:1px}");

    statusToolBar = new QToolBar(this);
    int iconHeight = 24; // statusBar()->height();
    int iconWidth = 36; // iconHeight * 3 / 2;
    statusToolBar->setIconSize(QSize(iconWidth, iconHeight));
    statusToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    statusToolBar->addAction(The::globalActions()->value("shufflePlaylist"));
    statusToolBar->addAction(The::globalActions()->value("repeatPlaylist"));
    statusToolBar->addAction(The::globalActions()->value("clearPlaylist"));
    statusBar()->addPermanentWidget(statusToolBar);

    statusBar()->show();
}

void MainWindow::readSettings() {
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    m_maximized = isMaximized();
    audioOutput->setVolume(settings.value("volume", 1).toDouble());
    audioOutput->setMuted(settings.value("volumeMute").toBool());
}

void MainWindow::writeSettings() {
    // do not save geometry when in full screen
    if (m_fullscreen) return;

    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("volume", audioOutput->volume());
    settings.setValue("volumeMute", audioOutput->isMuted());
    mediaView->saveSplitterState();
}

void MainWindow::goBack() {
    if ( history->size() > 1 ) {
        history->pop();
        QWidget *widget = history->pop();
        showWidget(widget);
    }
}

void MainWindow::showWidget(QWidget* widget) {

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
        QString windowTitle = metadata.value("title").toString();
        if (windowTitle.length())
            windowTitle += " - ";
        setWindowTitle(windowTitle + Constants::APP_NAME);
        statusBar()->showMessage((metadata.value("description").toString()));
    }

    fullscreenAct->setEnabled(widget == mediaView);
    aboutAct->setEnabled(widget != aboutView);
    chooseFolderAct->setEnabled(widget != chooseFolderView && widget != collectionScannerView);

    // toolbar only for the mediaView
    mainToolBar->setVisible(widget == mediaView || widget == contextualView);
    // statusToolBar->setVisible(widget == mediaView || widget == contextualView);
    statusBar()->setVisible(widget == mediaView || widget == contextualView);

#ifdef Q_WS_MAC
    // crossfade only on OSX
    // where we can be sure of video performance
    fadeInWidget(views->currentWidget(), widget);
#endif

    views->setCurrentWidget(widget);

    history->push(widget);

    setUpdatesEnabled(true);

}

void MainWindow::fadeInWidget(QWidget *oldWidget, QWidget *newWidget) {
    /*
    qDebug() << "MainWindow::fadeInWidget";
    qDebug() << "widgets" << oldWidget << newWidget;
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
    writeSettings();
    qApp->quit();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    quit();
    QWidget::closeEvent(event);
}

void MainWindow::showChooseFolderView() {
    if (!chooseFolderView) {
        chooseFolderView = new ChooseFolderView(this);
        connect(chooseFolderView, SIGNAL(locationChanged(QString)), SLOT(startFullScan(QString)));
        views->addWidget(chooseFolderView);
    }
    showWidget(chooseFolderView);
}

void MainWindow::showMediaView() {
    showWidget(mediaView);
}

void MainWindow::toggleContextualView() {
    if (!contextualView) {
        contextualView = new ContextualView(this);
        views->addWidget(contextualView);
    }
    if (views->currentWidget() == contextualView) {
        goBack();
        contextualAct->setChecked(false);

        QList<QKeySequence> shortcuts;
        shortcuts << QKeySequence(Qt::CTRL + Qt::Key_Space);
        contextualAct->setShortcuts(shortcuts);
        stopAct->setShortcut(QKeySequence(Qt::Key_Escape));

    } else {
        Track *track = mediaView->getPlaylistModel()->activeTrack();
        if (track) {
            contextualView->setTrack(track);
            showWidget(contextualView);
            contextualAct->setChecked(true);

            stopAct->setShortcut(QString(""));
            QList<QKeySequence> shortcuts;
            // for some reason it is important that ESC comes first
            shortcuts << QKeySequence(Qt::Key_Escape) << contextualAct->shortcuts();
            contextualAct->setShortcuts(shortcuts);
        }
    }
}

void MainWindow::updateContextualView(Track *track) {
    if (views->currentWidget() == contextualView) {
        contextualView->setTrack(track);
    }
}

void MainWindow::startFullScan(QString dir) {
    // qDebug() << "startFullScan" << dir;

    QSettings settings;
    settings.setValue("collectionRoot", dir);

    if (!collectionScannerView) {
        collectionScannerView = new CollectionScannerView(this);
        views->addWidget(collectionScannerView);
    }
    showWidget(collectionScannerView);
    collectionScannerView->startScan(dir);
}

void MainWindow::startIncrementalScan() {
    QSettings settings;
    QString dir = settings.value("collectionRoot").toString();

    CollectionScannerThread &scanner = CollectionScannerThread::instance();
    // connect(scanner, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)), Qt::QueuedConnection);
    // connect(scanner, SIGNAL(finished()), window(), SLOT(showMediaView()));
    // connect(scanner, SIGNAL(finished()), SLOT(scanFinished()));
    scanner.setDirectory(QDir(dir));
    scanner.setIncremental(true);
    scanner.start();
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */) {

    // qDebug() << "Phonon state: " << newState;

    // play action
    if (newState == Phonon::PlayingState) {
        playAct->setIcon(QtIconLoader::icon("media-playback-pause", QIcon(":/images/media-playback-pause.png")));
        playAct->setText(tr("&Pause"));
        playAct->setStatusTip(tr("Pause playback"));
    } else if (newState == Phonon::StoppedState || newState == Phonon::PausedState ) {
        playAct->setIcon(QtIconLoader::icon("media-playback-start", QIcon(":/images/media-playback-start.png")));
        playAct->setText(tr("&Play"));
        playAct->setStatusTip(tr("Resume playback"));
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
        stopAct->setEnabled(true);
        contextualAct->setEnabled(true);
        break;

         case Phonon::StoppedState:
        stopAct->setEnabled(false);
        break;

         case Phonon::PausedState:
        stopAct->setEnabled(true);
        break;

         case Phonon::BufferingState:
         case Phonon::LoadingState:
        stopAct->setEnabled(true);
        currentTime->clear();
        totalTime->clear();
        break;

         default:
        contextualAct->setEnabled(false);
    }
}

void MainWindow::stop() {
    // TODO mediaView->stop();
    mediaObject->stop();
    currentTime->clear();
    // totalTime->clear();
}

void MainWindow::playPause() {
    // qDebug() << "pause() called" << mediaObject->state();
    switch( mediaObject->state() ) {
    case Phonon::PlayingState:
        mediaObject->pause();
        break;
    default:
        mediaObject->play();
        break;
    }
}

void MainWindow::fullscreen() {

    setUpdatesEnabled(false);

    if (m_fullscreen) {
        // use setShortcuts instead of setShortcut
        // the latter seems not to work
        QList<QKeySequence> shortcuts;
        shortcuts << QKeySequence(Qt::ALT + Qt::Key_Return);
        fullscreenAct->setShortcuts(shortcuts);
        fullscreenAct->setText(tr("&Full Screen"));
        stopAct->setShortcut(QKeySequence(Qt::Key_Escape));
        if (m_maximized) showMaximized();
        else showNormal();
    } else {
        stopAct->setShortcut(QString(""));
        QList<QKeySequence> shortcuts;
        // for some reason it is important that ESC comes first
        shortcuts << QKeySequence(Qt::Key_Escape) << QKeySequence(Qt::ALT + Qt::Key_Return);
        fullscreenAct->setShortcuts(shortcuts);
        fullscreenAct->setText(tr("Exit &Full Screen"));
        m_maximized = isMaximized();

        // save geometry now, if the user quits when in full screen
        // geometry won't be saved
        writeSettings();

        showFullScreen();
    }

    // mainToolBar->setVisible(m_fullscreen);
    // statusBar()->setVisible(m_fullscreen);
    menuBar()->setVisible(m_fullscreen);

    m_fullscreen = !m_fullscreen;

    setUpdatesEnabled(true);
}

void MainWindow::searchFocus() {
    QWidget *view = views->currentWidget();
    if (view == mediaView) {
        toolbarSearch->setFocus();
    }
}

void MainWindow::initPhonon() {
    // Phonon initialization
    mediaObject = new Phonon::MediaObject(this);
    mediaObject->setTickInterval(250);
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            this, SLOT(stateChanged(Phonon::State, Phonon::State)));
    connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
    connect(mediaObject, SIGNAL(totalTimeChanged(qint64)), this, SLOT(totalTimeChanged(qint64)));
    audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    connect(audioOutput, SIGNAL(volumeChanged(qreal)), this, SLOT(volumeChanged(qreal)));
    connect(audioOutput, SIGNAL(mutedChanged(bool)), this, SLOT(volumeMutedChanged(bool)));
    Phonon::createPath(mediaObject, audioOutput);
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
    if (volumeSlider->audioOutput())
        volumeSlider->audioOutput()->setMuted(!volumeSlider->audioOutput()->isMuted());
}

void MainWindow::volumeChanged(qreal newVolume) {
    if (volumeSlider->audioOutput())
        // automatically unmute when volume changes
        if (volumeSlider->audioOutput()->isMuted())
            volumeSlider->audioOutput()->setMuted(false);
    statusBar()->showMessage(tr("Volume at %1%").arg(newVolume*100));
}

void MainWindow::volumeMutedChanged(bool muted) {
    if (muted)
        statusBar()->showMessage(tr("Volume is muted"));
    else
        statusBar()->showMessage(tr("Volume is unmuted"));
}

