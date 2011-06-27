#ifndef COLLECTIONSUGGESTER_H
#define COLLECTIONSUGGESTER_H

#include "suggester.h"

class CollectionSuggester : public Suggester {

    Q_OBJECT

public:
    CollectionSuggester(QObject *parent = 0);
    void suggest(QString query);

signals:
    void ready(QStringList);

};

#endif // COLLECTIONSUGGESTER_H
