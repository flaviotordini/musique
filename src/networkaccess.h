#ifndef NETWORKACCESS_H
#define NETWORKACCESS_H

#include <QtNetwork>
#include "constants.h"

static const QString USER_AGENT = QString(Constants::NAME)
                                  + " " + Constants::VERSION
                                  + " (" + Constants::WEBSITE + ")";

namespace The {
    QNetworkAccessManager* networkAccessManager();
}

class NetworkReply : public QObject {

    Q_OBJECT

public:
    NetworkReply(QNetworkReply* networkReply);
    bool followRedirects;

public slots:
    void finished();
    void requestError(QNetworkReply::NetworkError);

signals:
    void data(QByteArray);
    void error(QNetworkReply*);
    void finished(QNetworkReply*);

private slots:
    void abort();

private:
    QNetworkReply *networkReply;
    QTimer *timer;

};


class NetworkAccess : public QObject {

    Q_OBJECT

public:
    NetworkAccess( QObject* parent=0);
    QNetworkReply* simpleGet(QUrl url,
                             int operation = QNetworkAccessManager::GetOperation,
                             const QByteArray& body = QByteArray());
    NetworkReply* get(QUrl url);
    NetworkReply* head(QUrl url);
    NetworkReply* post(QUrl url, const QMap<QString, QString>& params);

private slots:
    void error(QNetworkReply::NetworkError);

};

typedef QPointer<QObject> ObjectPointer;
Q_DECLARE_METATYPE(ObjectPointer)

#endif // NETWORKACCESS_H
