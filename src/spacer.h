#ifndef SPACER_H
#define SPACER_H

#include <QtWidgets>

class Spacer : public QWidget {

public:
    Spacer(QWidget *parent = 0);
    Spacer(int width, QWidget *parent = 0);
    void setWidth(int width) { this->width = width; }

protected:
    QSize sizeHint() const;
    int width;
};

#endif // SPACER_H
