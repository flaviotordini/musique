CONFIG += c++17 exceptions_off rtti_off object_parallel_to_source

TEMPLATE = app
VERSION = 1.12.1
DEFINES += APP_VERSION="$$VERSION"

APP_NAME = Musique
DEFINES += APP_NAME="$$APP_NAME"

APP_UNIX_NAME = musique
DEFINES += APP_UNIX_NAME="$$APP_UNIX_NAME"

BUILD_YEAR = $$str_member($${_DATE_}, -4, -1)
DEFINES += BUILD_YEAR="$$BUILD_YEAR"

message(Building $${APP_NAME} $${VERSION})
message(Qt $$[QT_VERSION] in $$[QT_INSTALL_PREFIX])

CONFIG -= debug_and_release
CONFIG(debug, debug|release): {
    message(Building for debug)
}
CONFIG(release, debug|release): {
    message(Building for release)
    DEFINES *= QT_NO_DEBUG_OUTPUT
    CONFIG += optimize_full
}

DEFINES *= QT_USE_QSTRINGBUILDER

TARGET = $${APP_UNIX_NAME}

QT += network sql widgets

include(lib/qt-reusable-widgets/qt-reusable-widgets.pri)
include(lib/http/http.pri)
include(lib/idle/idle.pri)
include(lib/js/js.pri)
include(lib/sharedcache/sharedcache.pri)

DEFINES += MEDIA_MPV MEDIA_AUDIOONLY
include(lib/media/media.pri)

!mac {
    DEFINES += QAPPLICATION_CLASS=QApplication
    include(lib/singleapplication/singleapplication.pri)
}

include(src/tags/tags.pri)

INCLUDEPATH += $$PWD/src

HEADERS += src/mainwindow.h \
    src/aboutview.h \
    src/infoview.h \
    src/lyrics.h \
    src/searchlineedit.h \
    src/constants.h \
    src/finderwidget.h \
    src/collectionscannerview.h \
    src/collectionscanner.h \
    src/database.h \
    src/model/track.h \
    src/model/item.h \
    src/model/album.h \
    src/model/artist.h \
    src/datautils.h \
    src/artistsqlmodel.h \
    src/mediaview.h \
    src/finderitemdelegate.h \
    src/albumsqlmodel.h \
    src/artistlistview.h \
    src/albumlistview.h \
    src/playlistmodel.h \
    src/trackmimedata.h \
    src/playlistview.h \
    src/collectionscannerthread.h \
    src/playlistwidget.h \
    src/choosefolderview.h \
    src/filesystemfinderview.h \
    src/model/folder.h \
    src/basesqlmodel.h \
    src/filesystemmodel.h \
    src/context/artistinfo.h \
    src/context/albuminfo.h \
    src/context/trackinfo.h \
    src/tracksqlmodel.h \
    src/tracklistview.h \
    src/playlistitemdelegate.h \
    src/droparea.h \
    src/filteringfilesystemmodel.h \
    src/globalshortcuts.h \
    src/globalshortcutbackend.h \
    src/suggester.h \
    src/autocomplete.h \
    src/searchview.h \
    src/searchmodel.h \
    src/collectionsuggester.h \
    src/coverutils.h \
    src/lastfmlogindialog.h \
    src/lastfm.h \
    src/imagedownloader.h \
    src/httputils.h \
    src/tagchecker.h \
    src/toolbarmenu.h \
    src/model/genre.h \
    src/genresmodel.h \
    src/genres.h \
    src/model/decade.h \
    src/finderlistview.h \
    src/waitingspinnerwidget.h
SOURCES += src/main.cpp \
    src/infoview.cpp \
    src/lyrics.cpp \
    src/mainwindow.cpp \
    src/aboutview.cpp \
    src/searchlineedit.cpp \
    src/finderwidget.cpp \
    src/collectionscannerview.cpp \
    src/collectionscanner.cpp \
    src/database.cpp \
    src/model/track.cpp \
    src/model/album.cpp \
    src/model/artist.cpp \
    src/datautils.cpp \
    src/artistsqlmodel.cpp \
    src/mediaview.cpp \
    src/finderitemdelegate.cpp \
    src/albumsqlmodel.cpp \
    src/artistlistview.cpp \
    src/albumlistview.cpp \
    src/playlistmodel.cpp \
    src/trackmimedata.cpp \
    src/playlistview.cpp \
    src/collectionscannerthread.cpp \
    src/playlistwidget.cpp \
    src/choosefolderview.cpp \
    src/filesystemfinderview.cpp \
    src/model/folder.cpp \
    src/basesqlmodel.cpp \
    src/filesystemmodel.cpp \
    src/context/artistinfo.cpp \
    src/context/albuminfo.cpp \
    src/context/trackinfo.cpp \
    src/tracksqlmodel.cpp \
    src/tracklistview.cpp \
    src/playlistitemdelegate.cpp \
    src/droparea.cpp \
    src/filteringfilesystemmodel.cpp \
    src/constants.cpp \
    src/globalshortcuts.cpp \
    src/globalshortcutbackend.cpp \
    src/autocomplete.cpp \
    src/searchview.cpp \
    src/searchmodel.cpp \
    src/collectionsuggester.cpp \
    src/coverutils.cpp \
    src/lastfmlogindialog.cpp \
    src/lastfm.cpp \
    src/imagedownloader.cpp \
    src/httputils.cpp \
    src/tagchecker.cpp \
    src/toolbarmenu.cpp \
    src/model/genre.cpp \
    src/genresmodel.cpp \
    src/genres.cpp \
    src/model/decade.cpp \
    src/finderlistview.cpp \
    src/waitingspinnerwidget.cpp

RESOURCES += resources.qrc
RESOURCES += $$files(icons/*.png, true)

DESTDIR = build/target/
OBJECTS_DIR = build/obj/
MOC_DIR = build/moc/
RCC_DIR = build/rcc/

# Tell Qt Linguist that we use UTF-8 strings in our sources
CODECFORTR = UTF-8
CODECFORSRC = UTF-8
include(locale/locale.pri)

# deploy
DISTFILES += CHANGES LICENSE
unix:!mac {
    DEFINES += APP_LINUX
    LIBS += -ltag
    INCLUDEPATH += /usr/include/taglib
    QT += dbus
    HEADERS += src/gnomeglobalshortcutbackend.h
    SOURCES += src/gnomeglobalshortcutbackend.cpp
    isEmpty(PREFIX):PREFIX = /usr/local
    BINDIR = $$PREFIX/bin
    INSTALLS += target
    target.path = $$BINDIR
    DATADIR = $$PREFIX/share
    PKGDATADIR = $$DATADIR/$${APP_UNIX_NAME}
    DEFINES += DATADIR=\\\"$$DATADIR\\\" \
        PKGDATADIR=\\\"$$PKGDATADIR\\\"
    INSTALLS += translations \
        desktop \
        iconsvg \
        icon16 \
        icon22 \
        icon32 \
        icon48 \
        icon64 \
        icon128 \
        icon256 \
        icon512
    translations.path = $$PKGDATADIR
    translations.files += $$DESTDIR/locale
    desktop.path = $$DATADIR/applications
    desktop.files += $${APP_UNIX_NAME}.desktop
    iconsvg.path = $$DATADIR/icons/hicolor/scalable/apps
    iconsvg.files += data/$${APP_UNIX_NAME}.svg
    icon16.path = $$DATADIR/icons/hicolor/16x16/apps
    icon16.files += data/16x16/$${APP_UNIX_NAME}.png
    icon22.path = $$DATADIR/icons/hicolor/22x22/apps
    icon22.files += data/22x22/$${APP_UNIX_NAME}.png
    icon32.path = $$DATADIR/icons/hicolor/32x32/apps
    icon32.files += data/32x32/$${APP_UNIX_NAME}.png
    icon48.path = $$DATADIR/icons/hicolor/48x48/apps
    icon48.files += data/48x48/$${APP_UNIX_NAME}.png
    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += data/64x64/$${APP_UNIX_NAME}.png
    icon128.path = $$DATADIR/icons/hicolor/128x128/apps
    icon128.files += data/128x128/$${APP_UNIX_NAME}.png
    icon256.path = $$DATADIR/icons/hicolor/256x256/apps
    icon256.files += data/256x256/$${APP_UNIX_NAME}.png
    icon512.path = $$DATADIR/icons/hicolor/512x512/apps
    icon512.files += data/512x512/$${APP_UNIX_NAME}.png
}
mac|win32|contains(DEFINES, APP_UBUNTU):include(local/local.pri)

message(QT: $$QT)
message(CONFIG: $$CONFIG)
message(DEFINES: $$DEFINES)
message(QMAKE_CXXFLAGS: $$QMAKE_CXXFLAGS)
message(QMAKE_LFLAGS: $$QMAKE_LFLAGS)
