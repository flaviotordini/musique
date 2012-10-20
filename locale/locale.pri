# This voodoo comes from the Arora project
INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

# ls -1 *.ts | tr '\n' ' '
TRANSLATIONS += ast.ts ca.ts cs_CZ.ts da.ts de.ts el.ts en.ts es.ts es_ES.ts fi_FI.ts fr.ts hu_HU.ts ia.ts it.ts nb.ts nl.ts pl.ts pt.ts pt_BR.ts ro.ts ru.ts sk.ts sr.ts tr.ts tt.ts uk.ts zh_CN.ts 

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

