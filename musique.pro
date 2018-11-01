CONFIG += c++14 exceptions_off rtti_off optimize_full
QMAKE_CXXFLAGS *= -fno-exceptions -fno-rtti
TEMPLATE = app
VERSION = 1.6
DEFINES += APP_VERSION="$$VERSION"

APP_NAME = Musique
DEFINES += APP_NAME="$$APP_NAME"

APP_UNIX_NAME = musique
DEFINES += APP_UNIX_NAME="$$APP_UNIX_NAME"

DEFINES *= QT_NO_DEBUG_OUTPUT
DEFINES *= QT_USE_QSTRINGBUILDER
DEFINES *= QT_STRICT_ITERATORS

TARGET = $${APP_UNIX_NAME}

QT += network sql widgets

include(lib/idle/idle.pri)

include(src/qtsingleapplication/qtsingleapplication.pri)
include(src/http/http.pri)
include(src/tags/tags.pri)

HEADERS += src/mainwindow.h \
    src/aboutview.h \
    src/view.h \
    src/searchlineedit.h \
    src/exlineedit.h \
    src/spacer.h \
    src/constants.h \
    src/updatechecker.h \
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
    src/basefinderview.h \
    src/playlistwidget.h \
    src/breadcrumbwidget.h \
    src/fontutils.h \
    src/choosefolderview.h \
    src/filesystemfinderview.h \
    src/model/folder.h \
    src/basesqlmodel.h \
    src/filesystemmodel.h \
    src/contextualview.h \
    src/context/artistinfo.h \
    src/context/albuminfo.h \
    src/context/trackinfo.h \
    src/tracksqlmodel.h \
    src/tracklistview.h \
    src/trackitemdelegate.h \
    src/playlistitemdelegate.h \
    src/droparea.h \
    src/minisplitter.h \
    src/filteringfilesystemmodel.h \
    src/globalshortcuts.h \
    src/globalshortcutbackend.h \
    src/suggester.h \
    src/autocomplete.h \
    src/searchview.h \
    src/searchmodel.h \
    src/collectionsuggester.h \
    src/diskcache.h \
    src/segmentedcontrol.h \
    src/coverutils.h \
    src/lastfmlogindialog.h \
    src/lastfm.h \
    src/imagedownloader.h \
    src/iconutils.h \
    src/appwidget.h \
    src/httputils.h \
    src/tagchecker.h \
    src/toolbarmenu.h
SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/aboutview.cpp \
    src/searchlineedit.cpp \
    src/exlineedit.cpp \
    src/spacer.cpp \
    src/updatechecker.cpp \
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
    src/basefinderview.cpp \
    src/playlistwidget.cpp \
    src/breadcrumbwidget.cpp \
    src/fontutils.cpp \
    src/choosefolderview.cpp \
    src/filesystemfinderview.cpp \
    src/model/folder.cpp \
    src/basesqlmodel.cpp \
    src/filesystemmodel.cpp \
    src/contextualview.cpp \
    src/context/artistinfo.cpp \
    src/context/albuminfo.cpp \
    src/context/trackinfo.cpp \
    src/tracksqlmodel.cpp \
    src/tracklistview.cpp \
    src/trackitemdelegate.cpp \
    src/playlistitemdelegate.cpp \
    src/droparea.cpp \
    src/minisplitter.cpp \
    src/filteringfilesystemmodel.cpp \
    src/constants.cpp \
    src/globalshortcuts.cpp \
    src/globalshortcutbackend.cpp \
    src/autocomplete.cpp \
    src/searchview.cpp \
    src/searchmodel.cpp \
    src/collectionsuggester.cpp \
    src/diskcache.cpp \
    src/segmentedcontrol.cpp \
    src/coverutils.cpp \
    src/lastfmlogindialog.cpp \
    src/lastfm.cpp \
    src/imagedownloader.cpp \
    src/iconutils.cpp \
    src/appwidget.cpp \
    src/httputils.cpp \
    src/tagchecker.cpp \
    src/toolbarmenu.cpp
RESOURCES += resources.qrc
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
    qt:greaterThan(QT_MAJOR_VERSION, 4) {
        LIBS += -lphonon4qt5
        INCLUDEPATH += /usr/include/phonon4qt5
    } else {
        QT += phonon
        INCLUDEPATH += /usr/include/phonon
    }
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
