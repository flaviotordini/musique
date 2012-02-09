#ifndef LASTFM_H
#define LASTFM_H

#include <QtCore>
#include <QtNetwork>

class LastFmLoginDialog;
class Track;

class LastFm : public QObject {

    Q_OBJECT

public:
    static LastFm& instance();
    const QString & getSessionKey() const { return sessionKey; }
    const QString & getUsername() const { return username; }
    bool isAuthorized() const { return !sessionKey.isEmpty(); }
    void authenticate(const QString& username, const QString& password);
    void logout();
    void nowPlaying(Track* track);
    void scrobble(Track* track);

signals:
    void authenticated();
    void error(QString message);

private slots:
    void authenticationResponse(QByteArray bytes);
    void authenticationError(QNetworkReply* reply);

private:
    void sign(QMap<QString, QString>& params);
    LastFm(QObject *parent = 0);
    QString sessionKey;
    QString username;

    LastFmLoginDialog* dialog;

};

#endif // LASTFM_H
