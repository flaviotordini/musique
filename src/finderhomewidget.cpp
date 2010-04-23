#include "finderhomewidget.h"
#include "constants.h"

FinderHomeWidget::FinderHomeWidget(QWidget *parent) : QWidget(parent) {

    setWindowTitle(tr("Home"));

    // speedup painting since we'll paint the whole background
    // by ourselves anyway in paintEvent()
    setAttribute(Qt::WA_OpaquePaintEvent);

    // colors
    QPalette p = palette();
    p.setBrush(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, QColor(Qt::white));
    p.setBrush(QPalette::Window, Qt::black);
    p.setColor(QPalette::WindowText, QColor(Qt::white));
    this->setPalette(p);

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QLabel *welcomeLabel =
            new QLabel("<h3>" +
                       tr("Welcome to <a href='%1'>%2</a>,")
                       .replace("<a ", "<a style='color:white'")
                       .arg(Constants::WEBSITE, Constants::APP_NAME)
                       + "</h3>" + tr("Use the icons below to explore your music collection."), this);
    welcomeLabel->setOpenExternalLinks(true);
    welcomeLabel->setMargin(15);
    layout->addWidget(welcomeLabel);
    // welcomeLabel->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:1,stop:0 #1a1a1a, stop:1 #343434)");

    QListWidget *items = new QListWidget(this);
    connect(items, SIGNAL(itemActivated(QListWidgetItem*)), SLOT(itemActivated(QListWidgetItem*)));
    items->setAutoFillBackground(false);
    items->setAttribute(Qt::WA_NoSystemBackground);
    items->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    items->setFrameShape(QFrame::NoFrame);
    items->setAttribute(Qt::WA_MacShowFocusRect, false);
    items->setSelectionMode(QAbstractItemView::NoSelection);

    items->addItem(tr("Artists"));
    items->addItem(tr("Albums"));
    items->addItem(tr("Tracks"));
    items->addItem(tr("Years"));
    items->addItem(tr("Languages"));

    layout->addWidget(items);

    /*
      Artists
      Albums
      Tracks
      Time machine
      By language
      Files
      ---
      Tagcloud
      */

}

void FinderHomeWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    QLinearGradient linearGrad(0, 0, 0, height());
    linearGrad.setColorAt(0, QColor(0x33, 0x33, 0x33));
    linearGrad.setColorAt(.2, QColor(0x43, 0x43, 0x43));
    painter.fillRect(0, 0, width(), height(), QBrush(linearGrad));
}

void FinderHomeWidget::itemActivated(QListWidgetItem *item) {

}
