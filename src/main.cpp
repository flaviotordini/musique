#include <QtGui>
#include <QNetworkReply>
#include <qtsingleapplication.h>
#include "constants.h"
#include "mainwindow.h"
#ifdef QT_MAC_USE_COCOA
#include "local/mac/mac_startup.h"
#endif
#ifdef APP_WIN
#include "qtwin.h"
#include "winsupport.h"
#endif

int main(int argc, char **argv) {

#ifdef QT_MAC_USE_COCOA
    mac::MacMain();
#endif

    QtSingleApplication app(argc, argv);
    if (app.sendMessage("Wake up!"))
        return 0;

    app.setApplicationName(Constants::APP_NAME);
    app.setOrganizationName(Constants::ORG_NAME);
    app.setOrganizationDomain(Constants::ORG_DOMAIN);
    app.setWheelScrollLines(1);
    // app.setQuitOnLastWindowClosed(false);

    const QString locale = QLocale::system().name();

    // qt translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + locale,
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // app translations
#ifdef PKGDATADIR
    QString dataDir = QLatin1String(PKGDATADIR);
#else
    QString dataDir = "";
#endif
    QString localeDir = dataDir + QDir::separator() + "locale";
    // if app was not "installed" use the app directory
    if (!QFile::exists(localeDir)) {
        localeDir = qApp->applicationDirPath() + QDir::separator() + "locale";
        // qDebug() << "Using locale dir" << localeDir << locale;
    }
    QTranslator translator;
    translator.load(locale, localeDir);
    app.installTranslator(&translator);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

    MainWindow mainWin;
    mainWin.setWindowTitle(Constants::APP_NAME);

#ifndef APP_MAC
    if (!QFile::exists(dataDir)) {
        dataDir = qApp->applicationDirPath() + "/data";
    }
    const int iconSizes [] = { 16, 22, 32, 48, 64, 128, 256, 512 };
    QIcon appIcon;
    for (int i = 0; i < 8; i++) {
        QString size = QString::number(iconSizes[i]);
        QString png = dataDir + "/" + size + "x" + size + "/minitunes.png";
        // qDebug() << png;
        appIcon.addFile(png, QSize(iconSizes[i], iconSizes[i]));
        // appIcon.addPixmap(QPixmap(png));
    }
    mainWin.setWindowIcon(appIcon);
#endif

#ifdef APP_WIN
    if (QtWin::isCompositionEnabled()) {
        QtWin::extendFrameIntoClientArea(&mainWin);
        mainWin.setContentsMargins(0, 0, 0, 0);
    }
    app.setFont(QFont("Segoe UI", 9));
#endif

    mainWin.show();

    app.setActivationWindow(&mainWin, true);

    // all string literals are UTF-8
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    // This is required in order to use QNetworkReply::NetworkError in QueuedConnetions
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");

    // Seed random number generator
    qsrand(QDateTime::currentDateTime().toTime_t());

    return app.exec();
}
