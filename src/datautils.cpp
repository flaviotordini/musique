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
    s.replace("_", " ");
    return s.simplified();
}

QString DataUtils::normalizeTag(QString s) {
    static const QRegExp nonWordExpression("\\W+");
    // static const QRegExp nonWordExpression("[^\\p{L}]+");

    s = s.simplified().toLower();

    // The Beatles => Beatles
    s.replace(QRegExp("^the "), "");

    // remove anything that is not a "letter"
    s.remove(nonWordExpression);

    return s;
}

QString DataUtils::md5(const QString name) {
    return QString::fromAscii(QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5).toHex());
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
