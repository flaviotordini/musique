#ifndef DATAUTILS_H
#define DATAUTILS_H

#include <QtCore>

class DataUtils {

public:
    static QString cleanTag(QString tag);
    static QString normalizeTag(QString tag);
    static QString md5(QString);
    static QString getXMLElementText(QByteArray bytes, QString element);
    static QString getXMLAttributeText(QByteArray bytes, QString element, QString attribute);
    static QString getSystemLanguageCode();

private:
    DataUtils();

};

#endif // DATAUTILS_H
