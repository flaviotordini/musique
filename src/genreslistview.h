#ifndef METAGENRESLISTVIEW_H
#define METAGENRESLISTVIEW_H

#include "finderlistview.h"
#include <QtWidgets>

class GenresListView : public FinderListView {
    Q_OBJECT

public:
    GenresListView(QWidget *parent = nullptr);
};

#endif // METAGENRESLISTVIEW_H
