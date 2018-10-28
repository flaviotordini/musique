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

#include "collectionscannerthread.h"
#include "collectionscanner.h"

CollectionScannerThread::CollectionScannerThread(QObject *parent) : QThread(parent) {
    // This will be used by Database to cache connections for this thread
    setObjectName("scanner" + QString::number(qrand()));
}

void CollectionScannerThread::run() {

    // qDebug() << "CollectionScannerThread::run()";

    scanner = new CollectionScanner(nullptr);
    scanner->setDirectory(rootDirectory);
    connect(scanner, SIGNAL(progress(int)), SIGNAL(progress(int)), Qt::QueuedConnection);
    connect(scanner, SIGNAL(error(QString)), SIGNAL(error(QString)), Qt::QueuedConnection);
    connect(scanner, SIGNAL(finished(QVariantMap)), SLOT(finish(QVariantMap)), Qt::QueuedConnection);
    // connect(scanner, SIGNAL(finished()), SIGNAL(finished()), Qt::QueuedConnection);
    scanner->run();

    // Start thread event loop
    // This makes signals and slots work
    exec();

    // qDebug() << "CollectionScannerThread::run() exited";

}

void CollectionScannerThread::setDirectory(QString directory) {
    rootDirectory = directory;
}

void CollectionScannerThread::cleanup() {
    qDebug() << "Cleanup";
    delete scanner;
    exit();
}

void CollectionScannerThread::finish(const QVariantMap &stats) {
    // qDebug() << "Finish";
    emit finished(stats);
    QTimer::singleShot(0, this, SLOT(cleanup()));
}
