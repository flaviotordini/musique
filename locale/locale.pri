# This voodoo comes from the Arora project
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

# ls -1 *.ts | tr '\n' ' '
TRANSLATIONS += ca.ts de.ts el.ts es_ES.ts fr.ts hu_HU.ts ia.ts it.ts lv.ts nb.ts pl.ts pt.ts pt_BR.ts ru.ts sr.ts tr.ts uk.ts 

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

