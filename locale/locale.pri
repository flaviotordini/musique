# This voodoo comes from the Arora project
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
TRANSLATIONS += 
isEmpty(QMAKE_LRELEASE) { 
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
updateqm.input = TRANSLATIONS
updateqm.output = build/target/locale/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE \
    ${QMAKE_FILE_IN} \
    -qm \
    build/target/locale/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link \
    target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

