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

#include "lastfm.h"
#include "datautils.h"
#include "networkaccess.h"
#include "lastfmlogindialog.h"
#include "mainwindow.h"
#include "constants.h"
#include "model/track.h"
#include "model/artist.h"
#include "model/album.h"

namespace The {
NetworkAccess* http();
}

static const QString WS = "http://ws.audioscrobbler.com/2.0/";

static LastFm* i = 0;

LastFm& LastFm::instance() {
    if (!i) i = new LastFm();
    return *i;
}

LastFm::LastFm(QObject *parent) : QObject(parent), dialog(0) {
    QSettings settings;
    username = settings.value("lastFmUsername").toString();
    sessionKey = settings.value("lastFmSessionKey").toString();
}

void LastFm::sign(QMap<QString, QString>& params) {
    QString s;
    QMapIterator<QString, QString> i(params);
    while (i.hasNext()) {
        i.next();
        s += i.key() + i.value();
    }
    s += Constants::LASTFM_SHARED_SECRET;
    params["api_sig"] = DataUtils::md5(s);
}

void LastFm::authenticate(const QString &username, const QString &password) {
    QSettings settings;
    settings.setValue("lastFmUsername", username);
    this->username = username;

    logout();

    QMap<QString, QString> params;
    params["method"] = "auth.getMobileSession";
    params["username"] = username;
    params["authToken"] = DataUtils::md5((username + DataUtils::md5(password)));
    params["api_key"] = Constants::LASTFM_API_KEY;
    params["lang"] = QLocale().name().left(2).toLower();
    sign(params);

    QUrl url(WS);
    QObject* reply = The::http()->post(url, params);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(authenticationResponse(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SLOT(authenticationError(QNetworkReply*)));
}

void LastFm::authenticationResponse(QByteArray bytes) {
    QXmlStreamReader xml(bytes);

    while(!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if(token == QXmlStreamReader::StartElement && xml.name() == "session") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "name") {
                    username = xml.readElementText();
                } else if (xml.name() == "key") {
                    sessionKey = xml.readElementText();
                }
            }
        }

    }

    if(xml.hasError()) {
        qWarning() << xml.errorString();
        emit error(xml.errorString());
    }

    if (!sessionKey.isEmpty()) {
        QSettings settings;
        settings.setValue("lastFmSessionKey", sessionKey);
        emit authenticated();
    } else qDebug() << "Missing sessionKey";

}

void LastFm::authenticationError(QNetworkReply * /*reply*/) {

    /*
    QXmlStreamReader xml(reply->readAll());

    QString errorMessage;

    while(!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if(token == QXmlStreamReader::StartElement && xml.name() == "error") {
            errorMessage = xml.readElementText();
        }

    }

    if(xml.hasError()) {
        qWarning() << xml.errorString();
        emit error(xml.errorString());
    }
    */

    emit error(tr("Authentication failed"));
}

void LastFm::scrobble(Track* track) {
    if (!track) return;
    if (sessionKey.isEmpty()) {
        qWarning() << "Not authenticated to Last.fm";
        return;
    }

    QUrl url(WS);

    QMap<QString, QString> params;
    params["method"] = "track.scrobble";

    params["timestamp"] = QString::number(track->getStartTime());

    params["track"] = track->getTitle();

    Artist* artist = track->getArtist();
    if (!artist) {
        qDebug() << __FUNCTION__ << "Missing artist for" << track;
        return;
    }
    params["artist"] = artist->getName();

    Album* album = track->getAlbum();
    if (album)
        params["album"] = album->getTitle();

    if (track->getNumber())
        params["trackNumber"] = QString::number(track->getNumber());

    if (track->getLength())
        params["duration"] = QString::number(track->getLength());

    params["api_key"] = Constants::LASTFM_API_KEY;
    params["sk"] = sessionKey;

    sign(params);

    The::http()->post(url, params);

}

void LastFm::nowPlaying(Track* track) {
    if (!track) return;
    if (sessionKey.isEmpty()) {
        qWarning() << "Not authenticated to Last.fm";
        return;
    }

    QUrl url(WS);

    QMap<QString, QString> params;
    params["method"] = "track.updateNowPlaying";
    params["track"] = track->getTitle();

    Artist* artist = track->getArtist();
    if (!artist) {
        qDebug() << __FUNCTION__ << "Missing artist for" << track;
        return;
    }
    params["artist"] = artist->getName();

    Album* album = track->getAlbum();
    if (album)
        params["album"] = album->getTitle();

    if (track->getNumber())
        params["trackNumber"] = QString::number(track->getNumber());

    if (track->getLength())
        params["duration"] = QString::number(track->getLength());

    params["api_key"] = Constants::LASTFM_API_KEY;
    params["sk"] = sessionKey;

    sign(params);

    The::http()->post(url, params);
}

void LastFm::logout() {
    sessionKey.clear();
    QSettings settings;
    settings.remove("lastFmSessionKey");
}
