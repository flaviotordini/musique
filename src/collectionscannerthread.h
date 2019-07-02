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

#ifndef COLLECTIONSCANNERTHREAD_H
#define COLLECTIONSCANNERTHREAD_H

#include <QtCore>
class CollectionScanner;

class CollectionScannerThread : public QThread {

    Q_OBJECT

public:
    ~CollectionScannerThread();
    static CollectionScannerThread &instance();
    void setDirectory(QString directory);
    void run();

signals:
    void progress(int);
    void error(QString message);
    void finished(const QVariantMap &stats);

private slots:
    void finish(const QVariantMap &stats);
    void cleanup();

private:
    CollectionScannerThread(QObject *parent = nullptr);

    QString rootDirectory;
    CollectionScanner* scanner;

};

#endif // COLLECTIONSCANNERTHREAD_H
