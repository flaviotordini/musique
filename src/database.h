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

#ifndef DATABASE_H
#define DATABASE_H

#include <QtCore>
#include <QtSql>

enum DatabaseStatus {
    ScanComplete = 1,
    ScanIncomplete
};

class Database : public QObject {

    Q_OBJECT

public:
    static Database& instance();
    QSqlDatabase getConnection();
    ~Database();
    void create();
    void drop();
    void clear();
    int status();
    void setStatus(int status);
    uint lastUpdate();
    void setLastUpdate(uint date);
    QString collectionRoot();
    void setCollectionRoot(const QString& dir);
    void closeConnections();
    void closeConnection();
    const QString &needsUpdate() { return updateRoot; }

    static const QString &getDataLocation();
    static const QString &getFilesLocation();
    static const QString &getDbLocation();

private:
    Database();
    void createAttributes();
    QVariant getAttribute(const QString& name);
    void setAttribute(const QString& name, const QVariant& value);
    bool removeRecursively(const QString & dirName);

    QMutex lock;
    QHash<QThread*, QSqlDatabase> connections;
    QString updateRoot;
};

#endif // DATABASE_H
