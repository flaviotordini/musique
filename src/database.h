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
    int status();
    void setStatus(int status);
    uint lastUpdate();
    void setLastUpdate(uint date);
    QString collectionRoot();
    void setCollectionRoot(QString dir);
    void drop();
    void closeConnections();

private:
    Database();
    void createDatabase();
    QVariant getAttribute(QString name);
    void setAttribute(QString name, QVariant value);

    QMutex lock;
    QString dbLocation;
    QHash<QThread*, QSqlDatabase> connections;

};

#endif // DATABASE_H
