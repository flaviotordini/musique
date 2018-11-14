#ifndef METAGENRESLISTVIEW_H
#define METAGENRESLISTVIEW_H

#include "basefinderview.h"
#include <QtWidgets>

class GenresListView : public BaseFinderView {
    Q_OBJECT

public:
    GenresListView(QWidget *parent = nullptr);
};

#endif // METAGENRESLISTVIEW_H
