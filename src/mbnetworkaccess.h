#ifndef MBNETWORKACCESS_H
#define MBNETWORKACCESS_H

#include "networkaccess.h"

class MBNetworkAccess : public QObject {

    Q_OBJECT

public:
    MBNetworkAccess();
    QObject* get(QUrl url);

signals:
    void data(QByteArray);
    void error(QNetworkReply*);

private slots:
    QObject* get();
    QObject* reallyGet();

private:
    QTimer *rateTimer;

};

#endif // MBNETWORKACCESS_H
