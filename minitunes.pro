CONFIG += release
TEMPLATE = app

# On some distro, Phonon headers cannot be found
INCLUDEPATH += /usr/include/phonon
INCLUDEPATH += /usr/include/taglib

# Saner string behaviour
# DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_STRICT_ITERATORS
TARGET = minitunes
mac { 
    TARGET = Minitunes
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
    LIBS += -framework TagLib
    LIBS += /Library/Frameworks/TagLib.framework/TagLib.dylib
    INCLUDEPATH += /Library/Frameworks/TagLib.framework/Headers
}
else:LIBS += -ltag
QT += network \
    xml \
    phonon \
    sql
include(src/qtsingleapplication/qtsingleapplication.pri)
include(src/thlibrary/thlibrary.pri)
HEADERS += src/mainwindow.h \
    src/settingsview.h \
    src/aboutview.h \
    src/view.h \
    src/searchlineedit.h \
    src/urllineedit.h \
    src/spacer.h \
    src/constants.h \
    src/iconloader/qticonloader.h \
    src/faderwidget/faderwidget.h \
    src/networkaccess.h \
    src/global.h \
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
    src/searchcompletion.h \
    src/mbnetworkaccess.h \
    src/collectionscannerthread.h \
    src/basefinderview.h \
    src/playlistwidget.h \
    src/breadcrumbwidget.h \
    src/finderhomewidget.h \
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
    src/playlistitemdelegate.h
SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/settingsview.cpp \
    src/aboutview.cpp \
    src/searchlineedit.cpp \
    src/urllineedit.cpp \
    src/spacer.cpp \
    src/iconloader/qticonloader.cpp \
    src/faderwidget/faderwidget.cpp \
    src/updatechecker.cpp \
    src/networkaccess.cpp \
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
    src/searchcompletion.cpp \
    src/mbnetworkaccess.cpp \
    src/collectionscannerthread.cpp \
    src/basefinderview.cpp \
    src/playlistwidget.cpp \
    src/breadcrumbwidget.cpp \
    src/finderhomewidget.cpp \
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
    src/playlistitemdelegate.cpp
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
DISTFILES += CHANGES \
    LICENSE
mac { 
    CONFIG += x86 \
        ppc
    QMAKE_INFO_PLIST = Info.plist
    ICON = minitunes.icns
}
unix { 
    isEmpty(PREFIX):PREFIX = /usr/local
    BINDIR = $$PREFIX/bin
    INSTALLS += target
    target.path = $$BINDIR
    DATADIR = $$PREFIX/share
    PKGDATADIR = $$DATADIR/minitube
    DEFINES += DATADIR=\\\"$$DATADIR\\\" \
        PKGDATADIR=\\\"$$PKGDATADIR\\\"
    INSTALLS += translations \
        desktop \
        iconsvg \
        icon16 \
        icon32 \
        icon128
    translations.path = $$PKGDATADIR
    translations.files += $$DESTDIR/locale
    desktop.path = $$DATADIR/applications
    desktop.files += minitunes.desktop
    
    # iconxpm.path = $$DATADIR/pixmaps
    # iconxpm.files += data/minitube.xpm
    iconsvg.path = $$DATADIR/icons/hicolor/scalable/apps
    iconsvg.files += data/minitube.svg
    icon16.path = $$DATADIR/icons/hicolor/16x16/apps
    icon16.files += data/16x16/minitube.png
    icon32.path = $$DATADIR/icons/hicolor/32x32/apps
    icon32.files += data/32x32/minitube.png
    icon128.path = $$DATADIR/icons/hicolor/128x128/apps
    icon128.files += data/128x128/minitube.png
}
