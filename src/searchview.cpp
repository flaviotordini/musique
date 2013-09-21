#include "searchview.h"
#include "fontutils.h"

SearchView::SearchView(QWidget *parent) : BaseFinderView(parent) {

    /*
    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setAlignment(Qt::AlignTop);

    label = new QLabel();
    label->setPalette(p);
    label->setFont(FontUtils::big());
    label->setMargin(20);
    layout->addWidget(label);
    */


    // itemDelegate()->set
}

void SearchView::search(QString query) {
    // label->setText(tr("Search results for '%1'").arg("<b>" + query + "</b>"));
}

void SearchView::paintEvent(QPaintEvent *event) {
    QListView::paintEvent(event);

    if (model()->rowCount() == 0) {
        event->accept();

        QString emptyMessage = tr("Your search had no results.");

        QPainter painter(this->viewport());
        QPen textPen;
        textPen.setBrush(palette().mid());
        painter.setPen(textPen);
        painter.setFont(FontUtils::biggerBold());

        QSize textSize(QFontMetrics(painter.font()).size(Qt::TextSingleLine, emptyMessage));
        QPoint centerPoint((this->width()-textSize.width())/2,
                           ((this->height()-textSize.height())/2));
        QRect centerRect(centerPoint, textSize);
        QRect boundRect;
        painter.drawText(centerRect, Qt::AlignCenter, emptyMessage, &boundRect);
    }
}
