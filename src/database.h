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
    int status();
    void setStatus(int status);
    uint lastUpdate();
    void setLastUpdate(uint date);
    QString collectionRoot();
    void setCollectionRoot(QString dir);
    void closeConnections();
    void closeConnection();

    static QString getDataLocation();
    static QString getDbLocation();

private:
    Database();
    QVariant getAttribute(QString name);
    void setAttribute(QString name, QVariant value);
    bool removeRecursively(const QString & dirName);

    QMutex lock;
    QHash<QThread*, QSqlDatabase> connections;

};

#endif // DATABASE_H
