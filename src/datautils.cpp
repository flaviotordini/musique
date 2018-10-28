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

#include "datautils.h"

DataUtils::DataUtils() { }

QString DataUtils::cleanTag(QString s) {
    s.replace('_', ' ');
    return s.simplified();
}

QString DataUtils::normalizeTag(const QString &tag) {
    QString s = tag.simplified().toLower();

    // The Beatles => Beatles
    if (s.length() > 4 && s.startsWith(QLatin1String("the "))) s = s.remove(0, 4);

    // Wendy & Lisa => Wendy and Lisa
    s.replace(QLatin1String(" & "), QLatin1String("and"));

    // remove anything that is not a "letter"
    const int l = s.length();
    QString n;
    n.reserve(l);
    for (int i = 0; i < l; ++i) {
        const QChar c = s.at(i);
        if (c.isLetterOrNumber()) n.append(c);
        // else qDebug() << "Dropping char" << c;
    }

    return n;
}

QString DataUtils::simplify(const QString &s) {
    QString s2 = s;
    s2.replace(QString::fromUtf8("’"), QLatin1String("'"));
    s2.replace(QString::fromUtf8("…"), QLatin1String("..."));
    // Praise/Love => Praise / Love
    s2.replace(QRegExp("(\\S)/(\\S)"), QLatin1String("\\1 / \\2"));
    return s2;
}

QString DataUtils::md5(const QString name) {
    return QString::fromLatin1(QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString DataUtils::getXMLElementText(QByteArray bytes, QString elementName) {
    QXmlStreamReader xml(bytes);

    /* We'll parse the XML until we reach end of it.*/
    while(!xml.atEnd() && !xml.hasError()) {

        /* Read next element.*/
        QXmlStreamReader::TokenType token = xml.readNext();

        /*
        qDebug() << xml.name();
        foreach (QXmlStreamAttribute attribute, xml.attributes())
            qDebug() << attribute.name() << ":" << attribute.value();
            */

        /* If token is StartElement, we'll see if we can read it.*/
        if(token == QXmlStreamReader::StartElement
           && xml.name() == elementName) {
            QString text = xml.readElementText();
            // qDebug() << element << ":" << text;
            return text;
        }

    }

    /* Error handling. */
    if(xml.hasError()) {
        qDebug() << xml.errorString();
    }

    return QString();

}

QString DataUtils::getXMLAttributeText(QByteArray bytes, QString element, QString attribute) {
    QXmlStreamReader xml(bytes);
    while(!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if(token == QXmlStreamReader::StartElement && xml.name() == element) {
            return  xml.attributes().value(attribute).toString();
        }
    }
    if(xml.hasError()) {
        qDebug() << xml.errorString();
    }
    return QString();
}

QString DataUtils::getSystemLanguageCode() {
    static QString locale;
    if (locale.isNull()) {
        locale = QLocale::system().name();
        // case for system locales such as "C"
        if (locale.length() < 2) {
            locale = "en";
        } else {
            int underscoreIndex = locale.indexOf('_');
            if (underscoreIndex != -1) {
                locale = locale.left(underscoreIndex);
            }
        }
    }
    return locale;
}

QString DataUtils::formatDuration(uint secs) {
    uint d = secs;
    QString res;
    uint seconds = d % 60;
    d /= 60;
    uint minutes = d % 60;
    d /= 60;
    uint hours = d % 24;
    if (hours == 0)
        return res.sprintf("%d:%02d", minutes, seconds);
    return res.sprintf("%d:%02d:%02d", hours, minutes, seconds);
}
