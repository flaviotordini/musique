#include <QtGui>
#include <QNetworkReply>
#include <qtsingleapplication.h>
#include "constants.h"
#include "mainwindow.h"
#include "utils.h"
#ifdef APP_EXTRA
#include "extra.h"
#endif
#ifdef Q_WS_MAC
#include "mac_startup.h"
#endif

int main(int argc, char **argv) {

#ifdef Q_WS_MAC
    mac::MacMain();
    // https://bugreports.qt-project.org/browse/QTBUG-32789
    QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
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
    app.setWheelScrollLines(1);
    app.setAttribute(Qt::AA_DontShowIconsInMenus);

#ifdef APP_EXTRA
    Extra::appSetup(&app);
#else
    QFile cssFile(":/style.css");
    cssFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(cssFile.readAll());
    app.setStyleSheet(styleSheet);
#endif

    // qt translations
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // app translations
#ifdef PKGDATADIR
    QString dataDir = QLatin1String(PKGDATADIR);
#else
    QString dataDir = "";
#endif
    QString localeDir = qApp->applicationDirPath() + "/locale";
    if (!QDir(localeDir).exists()) {
        localeDir = dataDir + "/locale";
    }
    // qDebug() << "Using locale dir" << localeDir << locale;
    QTranslator translator;
    translator.load(QLocale::system(), QString(), QString(), localeDir);
    app.installTranslator(&translator);
    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

    MainWindow* mainWin = MainWindow::instance();
    mainWin->show();

#ifndef APP_MAC
    QIcon appIcon;
    if (QDir(dataDir).exists()) {
        appIcon = Utils::icon(Constants::UNIX_NAME);
    } else {
        dataDir = qApp->applicationDirPath() + "/data";
        const int iconSizes [] = { 16, 22, 32, 48, 64, 128, 256, 512 };
        for (int i = 0; i < 8; i++) {
            QString size = QString::number(iconSizes[i]);
            QString png = dataDir + "/" + size + "x" + size + "/" +
                    Constants::UNIX_NAME + ".png";
            appIcon.addFile(png, QSize(iconSizes[i], iconSizes[i]));
        }
    }
    if (appIcon.isNull()) {
        appIcon.addFile(":/images/app.png");
    }
    mainWin->setWindowIcon(appIcon);
#endif

    mainWin->connect(&app, SIGNAL(messageReceived(const QString &)),
                     mainWin, SLOT(messageReceived(const QString &)));
    app.setActivationWindow(mainWin, true);

    // all string literals are UTF-8
    // QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    // This is required in order to use QNetworkReply::NetworkError in QueuedConnetions
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");

    // Seed random number generator
    qsrand(QDateTime::currentDateTime().toTime_t());

    return app.exec();
}
