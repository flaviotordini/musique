/* $BEGIN_LICENSE

This file is part of Musique.
Copyright 2013, Flavio Tordini <flavio.tordini@gmail.com>

Musique is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Musique is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Musique.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#include <QtNetwork>
#include <QtWidgets>

#include "constants.h"
#include "http.h"
#include "httputils.h"
#include "iconutils.h"
#include "mainwindow.h"
#include "updateutils.h"
#include <qtsingleapplication.h>
#ifdef APP_EXTRA
#include "extra.h"
#endif
#ifdef Q_OS_MAC
#include "mac_startup.h"
#endif

int main(int argc, char **argv) {
    qSetMessagePattern("[%{function}] %{message}");

#ifdef Q_OS_MAC
    mac::MacMain();
#endif

    QCoreApplication::setApplicationName(Constants::NAME);
    QCoreApplication::setOrganizationName(Constants::ORG_NAME);
    QCoreApplication::setOrganizationDomain(Constants::ORG_DOMAIN);
    QCoreApplication::setApplicationVersion(Constants::VERSION);
    QGuiApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication::setWheelScrollLines(1);

    QtSingleApplication app(argc, argv);
    QString message;
    if (app.arguments().size() > 1) {
        message = app.arguments().at(1);
        if (message == QLatin1String("--help")) {
            MainWindow::printHelp();
            return 0;
        }
    }
    if (app.sendMessage(message)) return 0;

#ifdef APP_EXTRA
    Extra::appSetup(&app);
#else
    QFile cssFile(":/res/style.css");
    cssFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(cssFile.readAll());
    app.setStyleSheet(styleSheet);
#endif

    // qt translations
    QTranslator qtTranslator;
    qtTranslator.load(QLatin1String("qt_") + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // app translations
#ifdef PKGDATADIR
    QString dataDir = QLatin1String(PKGDATADIR);
#else
    QString dataDir;
#endif
#ifdef APP_MAC
    QString localeDir = qApp->applicationDirPath() + "/../Resources/locale";
#else
    QString localeDir = qApp->applicationDirPath() + "/locale";
#endif
    if (!QDir(localeDir).exists()) {
        localeDir = dataDir + "/locale";
    }
    // qDebug() << "Using locale dir" << localeDir << locale;
    QTranslator translator;
    translator.load(QLocale::system(), QString(), QString(), localeDir);
    app.installTranslator(&translator);

    UpdateUtils::init();

    MainWindow *mainWindow = MainWindow::instance();
    mainWindow->show();

#ifndef APP_MAC
    QIcon appIcon;
    if (QDir(dataDir).exists()) {
        appIcon = IconUtils::icon(Constants::UNIX_NAME);
    } else {
        dataDir = qApp->applicationDirPath() + "/data";
        const int iconSizes[] = {16, 22, 32, 48, 64, 128, 256, 512};
        for (int iconSize : iconSizes) {
            QString size = QString::number(iconSize);
            QString png = dataDir + '/' + size + 'x' + size + '/' + Constants::UNIX_NAME +
                          QLatin1String(".png");
            appIcon.addFile(png, QSize(iconSize, iconSize));
        }
    }
    if (appIcon.isNull()) {
        appIcon.addFile(":/images/app.png");
    }
    mainWindow->setWindowIcon(appIcon);
#endif

    mainWindow->connect(&app, SIGNAL(messageReceived(const QString &)), mainWindow,
                        SLOT(messageReceived(const QString &)));
    app.setActivationWindow(mainWindow, true);

    // This is required in order to use QNetworkReply::NetworkError in QueuedConnetions
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");

    Http::instance().addRequestHeader("User-Agent", HttpUtils::userAgent());

    // Seed random number generator
    qsrand(QDateTime::currentDateTimeUtc().toTime_t());

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    return app.exec();
}
