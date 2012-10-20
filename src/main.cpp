#include <QtGui>
#include <QNetworkReply>
#include <qtsingleapplication.h>
#include "constants.h"
#include "mainwindow.h"
#ifdef QT_MAC_USE_COCOA
#include "mac_startup.h"
#include "macfullscreen.h"
#endif
#ifdef APP_WIN
#include "winsupport.h"
#endif
#include "iconloader/qticonloader.h"

int main(int argc, char **argv) {

#ifdef QT_MAC_USE_COCOA
    mac::MacMain();
#endif

    QtSingleApplication app(argc, argv);
    QString message = app.arguments().size() > 1 ? app.arguments().at(1) : "";
    if (message == "--help") {
        MainWindow::printHelp();
        return 0;
    }
    if (app.sendMessage(message))
        return 0;

    app.setApplicationName(Constants::NAME);
    app.setOrganizationName(Constants::ORG_NAME);
    app.setOrganizationDomain(Constants::ORG_DOMAIN);
#ifndef APP_WIN
    app.setWheelScrollLines(1);
#endif

#ifdef APP_MAC
    QCoreApplication::setAttribute(Qt::AA_NativeWindows);
    QFile file(":/mac.css");
    file.open(QFile::ReadOnly);
    app.setStyleSheet(QLatin1String(file.readAll()));
#endif

#ifdef APP_WIN
    QFile file(":/win.css");
    file.open(QFile::ReadOnly);
    app.setStyleSheet(QLatin1String(file.readAll()));
#endif

    const QString locale = QLocale::system().name();

    // qt translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + locale,
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    // qWarning() << "Qt translations:" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    app.installTranslator(&qtTranslator);

    // app translations
#ifdef PKGDATADIR
    QString dataDir = QLatin1String(PKGDATADIR);
#else
    QString dataDir = "";
#endif
    QString localeDir = qApp->applicationDirPath() + QDir::separator() + "locale";
    if (!QDir(localeDir).exists()) {
        localeDir = dataDir + QDir::separator() + "locale";
    }
    QTranslator translator;
    translator.load(locale, localeDir);
    app.installTranslator(&translator);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

    MainWindow* mainWin = MainWindow::instance();
    mainWin->setWindowTitle(Constants::NAME);

#ifdef APP_MAC
    app.setQuitOnLastWindowClosed(false);
    mac::SetupFullScreenWindow(mainWin->winId());
#endif

#ifndef APP_MAC
    QIcon appIcon;
    if (QDir(dataDir).exists()) {
        appIcon = QtIconLoader::icon(Constants::UNIX_NAME);
    } else {
        dataDir = qApp->applicationDirPath() + "/data";
        const int iconSizes [] = { 16, 22, 32, 48, 64, 128, 256, 512 };
        for (int i = 0; i < 8; i++) {
            QString size = QString::number(iconSizes[i]);
            QString png = dataDir + "/" + size + "x" + size + "/" + Constants::UNIX_NAME + ".png";
            appIcon.addFile(png, QSize(iconSizes[i], iconSizes[i]));
        }
    }
    if (appIcon.isNull()) {
        appIcon.addFile(":/images/app.png");
    }
    mainWin->setWindowIcon(appIcon);
#endif

#ifdef APP_WIN
    app.setFont(QFont("Segoe UI", 9));
#endif

    mainWin->show();

    mainWin->connect(&app, SIGNAL(messageReceived(const QString &)), mainWin, SLOT(messageReceived(const QString &)));
    app.setActivationWindow(mainWin, true);

    // all string literals are UTF-8
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    // This is required in order to use QNetworkReply::NetworkError in QueuedConnetions
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");

    // Seed random number generator
    qsrand(QDateTime::currentDateTime().toTime_t());

    return app.exec();
}
