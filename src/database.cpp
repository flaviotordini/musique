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

#include "database.h"
#include "constants.h"

Database::Database() {
    QMutexLocker locker(&lock);

    const QString dataLocation = getDataLocation();
    const QString dbLocation = getDbLocation();

    if(QFile::exists(dbLocation)) {

        // check db version
        int databaseVersion = getAttribute("version").toInt();
        if (databaseVersion != Constants::DATABASE_VERSION) {
            qWarning("Updating database version: %d to %d", databaseVersion, Constants::DATABASE_VERSION);
            updateRoot = collectionRoot();
            drop();
            removeRecursively(dataLocation);
            create();
        }

    } else create();
}

Database::~Database() {
    closeConnections();
}

const QString &Database::getDataLocation() {
    static const QString location = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    return location;
}

const QString &Database::getFilesLocation() {
    static const QString location = getDataLocation() + QLatin1String("/files/");
    return location;
}

const QString &Database::getDbLocation() {
    static const QString location = getDataLocation() + QLatin1String("/") +
            QLatin1String(Constants::UNIX_NAME) + QLatin1String(".db");
    return location;
}

void Database::create() {
    qDebug() << "Creating the database";

    QDir().mkpath(getDataLocation());

    const QSqlDatabase db = getConnection();

    createAttributes();

    QSqlQuery("create table artists ("
              "id integer primary key autoincrement,"
              "hash varchar(32),"
              "name varchar(255),"
              "yearFrom integer,"
              "yearTo integer,"
              "listeners integer,"
              "albumCount integer,"
              "trackCount integer)", db);

    QSqlQuery("create table albums ("
              "id integer primary key autoincrement,"
              "hash varchar(32),"
              "title varchar(255),"
              "year integer,"
              "artist integer,"
              "listeners integer,"
              "trackCount integer)", db);

    QSqlQuery("create table tracks ("
              "id integer primary key autoincrement,"
              "path varchar(255)," // path is NOT unique: .cue files
              "title varchar(255),"
              // TODO "start integer, end integer," // cue files
              "duration integer,"
              "track integer,"
              "disk integer,"
              "diskCount integer,"
              "year integer,"
              "artist integer,"
              "album integer,"
              "tstamp integer)", db);

    QSqlQuery("create table nontracks ("
              "path varchar(255),"
              "tstamp integer)", db);
    QSqlQuery("create unique index unique_nontracks_path on nontracks(path)", db);

    QSqlQuery("create table downloads ("
              "id integer primary key autoincrement,"
              "objectid integer,"
              "type integer,"
              "errors integer,"
              "url varchar(255))", db);

    /* TODO tags
    QSqlQuery("create table tags ("
              "id integer primary key autoincrement,"
              "name varchar,"
              "artistCount integer,"
              "albumCount integer)", db);
    QSqlQuery("create unique index unique_tags_name on tags(name)", db);

    QSqlQuery("create table artisttags ("
              "artist integer,"
              "tag integer)", db);
    QSqlQuery("create unique index unique_artisttags on artisttags(artist,tag)", db);

    QSqlQuery("create table albumtags ("
              "album integer,"
              "tag integer)", db);
    QSqlQuery("create unique index unique_albumtags on albumtags(album,tag)", db);
    */
}

void Database::createAttributes() {
    const QSqlDatabase db = getConnection();
    QSqlQuery("create table attributes (name varchar(255), value)", db);
    QSqlQuery("create unique index unique_attributes_name on attributes(name)", db);
    QSqlQuery("insert into attributes (name, value) values ('version', " + QString::number(Constants::DATABASE_VERSION) + ")", db);
    QSqlQuery("insert into attributes (name, value) values ('status', " + QString::number(ScanIncomplete) + ")", db);
    QSqlQuery("insert into attributes (name, value) values ('lastUpdate', 0)", db);
    QSqlQuery("insert into attributes (name, value) values ('root', '')", db);
}

Database& Database::instance() {
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static Database *databaseInstance = new Database();
    return *databaseInstance;
}

QSqlDatabase Database::getConnection() {
    QThread *currentThread = QThread::currentThread();
    if (!currentThread) {
        qDebug() << "current thread is null";
        return QSqlDatabase();
    }

    const QString threadName = currentThread->objectName();
    // qDebug() << "threadName" << threadName << currentThread;
    if (connections.contains(currentThread)) {
        return connections.value(currentThread);
    } else {
        qDebug() << "Creating db connection for" << threadName;
        QSqlDatabase connection = QSqlDatabase::addDatabase("QSQLITE", threadName);
        connection.setDatabaseName(getDbLocation());
        if(!connection.open()) {
            qWarning() << QString("Cannot connect to database %1 in thread %2")
                          .arg(connection.databaseName(), threadName);
        }
        connections.insert(currentThread, connection);
        return connection;
    }
}

int Database::status() {
    QVariant status = getAttribute("status");
    if (status.isValid()) return status.toInt();
    else return ScanIncomplete;
}

void Database::setStatus(int status) {
    setAttribute("status", QVariant(status));
}

uint Database::lastUpdate() {
    return getAttribute("lastUpdate").toUInt();
}

void Database::setLastUpdate(uint date) {
    setAttribute("lastUpdate", QVariant(date));
}

QString Database::collectionRoot() {
    return getAttribute("root").toString();
}

void Database::setCollectionRoot(QString dir) {
    setAttribute("root", QVariant(dir));
}

QVariant Database::getAttribute(QString name) {
    QSqlQuery query("select value from attributes where name=?", getConnection());
    query.bindValue(0, name);

    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.boundValues().values() << query.lastError().text();
    if (query.next())
        return query.value(0);
    return QVariant();
}

void Database::setAttribute(QString name, QVariant value) {
    QSqlQuery query(getConnection());
    query.prepare("update attributes set value=? where name=?");
    query.bindValue(0, value);
    query.bindValue(1, name);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

void Database::drop() {
    qDebug() << "Dropping the database";

    if (!QFile::remove(getDbLocation())) {
        qWarning() << "Cannot delete database" << getDbLocation();

        // fallback to delete records in tables
        const QSqlDatabase db = getConnection();
        QSqlQuery query(db);
        if (!query.exec("select name from sqlite_master where type='table'")) {
            qWarning() << query.lastQuery() << query.lastError().text();
        }

        QStringList tableNames;
        while (query.next()) {
            QString tableName = query.value(0).toString();
            if (tableName.startsWith("sqlite_")) continue;
            tableNames << tableName;
        }

        foreach (QString tableName, tableNames) {
            QString dropSQL = "drop table " + tableName;
            QSqlQuery query2(db);
            if (!query2.exec(dropSQL)) {
                qWarning() << query2.lastQuery() << query2.lastError().text();
            }
        }

        query.exec("delete from sqlite_sequence");
        query.exec("vacuum");
    }

    closeConnections();
}

void Database::clear() {
    const QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    if (!query.exec("select name from sqlite_master where type='table'")) {
        qWarning() << query.lastQuery() << query.lastError().text();
    }

    while (query.next()) {
        QString tableName = query.value(0).toString();
        if (tableName.startsWith("sqlite_")) continue;
        QString dropSQL = "delete from " + tableName;
        QSqlQuery query2(db);
        if (!query2.exec(dropSQL)) {
            qWarning() << query2.lastQuery() << query2.lastError().text();
        }
    }

    query.exec("delete from sqlite_sequence");
    query.exec("vacuum");
    createAttributes();
}

void Database::closeConnections() {
    foreach(QSqlDatabase connection, connections.values()) {
        qDebug() << "Closing connection" << connection;
        connection.close();
    }
    connections.clear();
}

void Database::closeConnection() {
    QThread *currentThread = QThread::currentThread();
    if (!connections.contains(currentThread)) return;
    QSqlDatabase connection = connections.take(currentThread);
    qDebug() << "Closing connection" << connection;
    connection.close();
}

bool Database::removeRecursively(const QString & dirName) {
    bool result = false;
    QDir dir(dirName);
    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(
                      QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
                      QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir())
                result = removeRecursively(info.absoluteFilePath());
            else
                result = QFile::remove(info.absoluteFilePath());

            if (!result)
                return result;
        }
        result = dir.rmdir(dirName);
    }
    return result;
}
