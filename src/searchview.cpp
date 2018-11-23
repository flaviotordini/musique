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

#include "searchview.h"
#include "fontutils.h"

SearchView::SearchView(QWidget *parent) : FinderListView(parent) {

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

void SearchView::search(const QString& query) {
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
        painter.setFont(FontUtils::big());

        QSize textSize(QFontMetrics(painter.font()).size(Qt::TextSingleLine, emptyMessage));
        QPoint centerPoint((this->width()-textSize.width())/2,
                           ((this->height()-textSize.height())/2));
        QRect centerRect(centerPoint, textSize);
        QRect boundRect;
        painter.drawText(centerRect, Qt::AlignCenter, emptyMessage, &boundRect);
    }
}
