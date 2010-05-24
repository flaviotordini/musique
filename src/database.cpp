#include "database.h"
#include "constants.h"
#include <QDesktopServices>

static const QString dbName = "minitunes.db";
static Database *databaseInstance = 0;

Database::Database() {

    QString dataLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    QDir().mkpath(dataLocation);
    dbDiskLocation = dataLocation + "/" + dbName;

    /*
    QString dbMemoryDir = "/dev/shm";
    QDir().mkpath(dbMemoryDir);
    uint unixTime = QDateTime::currentDateTime().toTime_t();
    dbMemoryLocation = dbMemoryDir + "/" + dbName + "." + QString::number(unixTime);
    */

    QMutexLocker locker(&lock);

    if(QFile::exists(dbDiskLocation)) {

        qDebug() << "Database found in" << dbDiskLocation;


        /*
        QFile::remove(dbMemoryLocation);
        qDebug() << "Copying database from" << dbDiskLocation << "to" << dbMemoryLocation;
        if (!QFile::copy(dbDiskLocation, dbMemoryLocation)) {
            qDebug() << "Cannot copy database from" << dbDiskLocation << "to" << dbMemoryLocation;
        }*/

        // check db version
        int databaseVersion = getAttribute("version").toInt();
        if (databaseVersion != Constants::DATABASE_VERSION) {
            qDebug("Wrong database version: %d", databaseVersion);
        }

    } else {

        /*
        bool restored = false;
        if (QFile::exists(dbMemoryLocation)) {
            qDebug() << "Panic! Database is in" << dbMemoryLocation << "but not in" << dbDiskLocation << "! Copying...";
            if (!QFile::copy(dbMemoryLocation, dbDiskLocation)) {
                qDebug() << "Cannot copy database from" << dbMemoryLocation << "to" << dbDiskLocation;
            } else restored = true;
        }

        if (!restored) */
        createDatabase();

    }
}

Database::~Database() {
    foreach(QSqlDatabase connection, connections.values())
        connection.close();
    connections.clear();
    // toDisk();
    // QFile::remove(dbMemoryLocation);
}

void Database::createDatabase() {

    qDebug() << "Creating the database";

    // TODO create indexes

    const QSqlDatabase db = getConnection();

    QSqlQuery("create table artists ("
              "id integer primary key autoincrement,"
              "hash varchar(32),"
              "name varchar(255),"
              "mbid varchar(50),"
              "lifeBegin integer,"
              "lifeEnd integer,"
              "albumCount integer,"
              "trackCount integer)", db);

    QSqlQuery("create table albums ("
              "id integer primary key autoincrement,"
              "hash varchar(32),"
              "title varchar(255),"
              "year integer,"
              // "language varchar(5),"
              "artist integer,"
              "trackCount integer)"
              , db);

    QSqlQuery(
            "create table tracks ("
            "id integer primary key autoincrement,"
            "path varchar(255)," // path is NOT unique: .cue files
            "title varchar(255),"
            "start integer, end integer," // cue files
            "duration integer,"
            "track integer,"
            "year integer,"
            "artist integer,"
            "album integer,"
            "tstamp integer)"
            , db);

    QSqlQuery("create table attributes (name varchar(255), value)", db);
    QSqlQuery("insert into attributes (name, value) values ('version', " + QString::number(Constants::DATABASE_VERSION) + ")", db);
    QSqlQuery("insert into attributes (name, value) values ('status', " + QString::number(ScanIncomplete) + ")", db);
    QSqlQuery("insert into attributes (name, value) values ('lastUpdate', 0)", db);

}

Database& Database::instance() {
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!databaseInstance) databaseInstance = new Database();
    return *databaseInstance;
}

QSqlDatabase Database::getConnection() {
    const QString threadName = QThread::currentThread()->objectName();
    // qDebug() << "threadName" << threadName;
    if (connections.contains(QThread::currentThread())) {
        return connections.value(QThread::currentThread());
    } else {
        QSqlDatabase connection = QSqlDatabase::addDatabase("QSQLITE", threadName);
        connection.setDatabaseName(dbDiskLocation);
        if(!connection.open()) {
            qDebug() << QString("Cannot connect to database %1 for thread %2").arg(dbDiskLocation, threadName);
        }
        connections.insert(QThread::currentThread(), connection);
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


/**
  * After calling this method you have to reacquire a valid instance using instance()
  */
void Database::drop() {
    if (!QFile::remove(dbDiskLocation)) {
        qDebug() << "Cannot delete database" << dbDiskLocation;
    }
    if (databaseInstance) delete databaseInstance;
    databaseInstance = 0;
}

void Database::toDisk() {
    return;
    /*
    qDebug() << "Copying database from" << dbMemoryLocation << "to" << dbDiskLocation;
    uint unixTime = QDateTime::currentDateTime().toTime_t();
    // QFile::rename(dbDiskLocation, dbDiskLocation + "." + QString::number(unixTime));
    QFile::remove(dbDiskLocation);
    if (!QFile::copy(dbMemoryLocation, dbDiskLocation)) {
        qDebug() << "Cannot copy database from" << dbMemoryLocation << "to" << dbDiskLocation;
    }*/
}
