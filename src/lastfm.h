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
    void authenticationResponse(const QByteArray& bytes);
    void authenticationError(const QString &message);

private:
    void sign(QMap<QString, QString>& params);
    LastFm(QObject *parent = 0);
    QString sessionKey;
    QString username;

    LastFmLoginDialog* dialog;

};

#endif // LASTFM_H
