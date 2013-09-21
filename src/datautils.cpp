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
