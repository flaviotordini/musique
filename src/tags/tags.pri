CONFIG -= rtti_off
QMAKE_CXXFLAGS -= -fno-rtti

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/apeutils.h \
    $$PWD/asfutils.h \
    $$PWD/id3utils.h \
    $$PWD/mp4utils.h \
    $$PWD/vorbisutils.h \
    $$PWD/tags.h \
    $$PWD/tagutils.h

SOURCES += \
    $$PWD/tagutils.cpp

