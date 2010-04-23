#include "datautils.h"

DataUtils::DataUtils() { }

static const QRegExp nonWordExpression("\\W+");
// static const QRegExp nonWordExpression("[^\\p{L}]+");

/**
  * Clean a string removing all non-word chars
  * and fixing all uppercase chars
  * This function is intended to increase legibility
  */
QString DataUtils::cleanTag(QString s) {
    // s.replace("&", "and");
    s.replace("_", " ");

    s = s.replace(nonWordExpression, " ");

    s = s.simplified();

    // Fix all uppercase strings
    if (s == s.toUpper()) {
        s = s.toLower();
        s[0] = s[0].toUpper();
    }

    return s;
}

/**
  * Build a unique identifier for a string
  */
QString DataUtils::normalizeTag(QString s) {
    s = s.simplified().toLower();

    // The Beatles => Beatles
    s.replace(QRegExp("^the "), "");

    // sort words
    s.replace("_", "");
    // s.replace("&", "and");
    QStringList words = s.split(nonWordExpression, QString::SkipEmptyParts);
    words.sort();
    s = words.join("");

    // remove anything that is not a "letter"
    // s = s.replace(nonWordExpression, "");

    // s = s.simplified().toLower();

    // TODO simplify accented chars èé=>e etc

    return s;
}

/**
  * Md5 of a string
  */
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
