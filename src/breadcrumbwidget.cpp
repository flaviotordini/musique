#include "breadcrumbwidget.h"
#include "iconloader/qticonloader.h"

class BreadcrumbLabel : public QLabel {

public:
    BreadcrumbLabel(QWidget *parent) : QLabel(parent) {

    }

    QSize sizeHint() {
        QSize size = QLabel::sizeHint();
        qDebug() << size;
        size.setWidth(size.width() + 100);
        return size;
    }

    void paintEvent(QPaintEvent *event) {
        const int PADDING = height() / 4;
        QPolygon polygon;

        polygon << QPoint(0, 0) << QPoint(width() - PADDING, 0)
                << QPoint(width(), height() / 2)
                << QPoint(width() - PADDING, height())
                << QPoint(0, height())
                << QPoint(PADDING, height() / 2);

        /*
        polygon << QPoint(0, 0) << QPoint(width(), 0)
                << QPoint(width(), height())
                << QPoint(0, height())
                << QPoint(PADDING, height() / 2);
 */
        QPalette palette;
        QPainter painter(this);
        painter.setBrush(palette.mid());
        // painter.setPen(palette.color(QPalette::Dark));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(polygon);

        QLabel::paintEvent(event);
    }

};

BreadcrumbWidget::BreadcrumbWidget(QWidget *parent) : QToolBar(parent) {

    QPalette p = palette();
    p.setBrush(QPalette::Window, Qt::black);
    p.setBrush(QPalette::Text, Qt::white);
    p.setBrush(QPalette::WindowText, Qt::white);
    p.setBrush(QPalette::ButtonText, Qt::white);
    setPalette(p);

    /*
    QBoxLayout *layout = new QHBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignLeft);
    */

    // setToolButtonStyle(Qt::ToolButtonTextOnly
    // setStyleSheet("QToolBar {padding:0; margin:0; }");
    // QToolBar { background: black; color: white }"



    QAction* action = new QAction(QtIconLoader::icon("go-previous", QIcon(":/images/go-previous.png")), tr("&Back"), this);
    // action->setEnabled(false);
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
    action->setStatusTip(tr("Go back to the previous view"));
    connect(action, SIGNAL(triggered()), this, SLOT(goBack()));
    addAction(action);


    /*
    action = new QAction(QtIconLoader::icon("go-home", QIcon(":/images/go-home.png")), tr("&Home"), this);
    // action->setEnabled(false);
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Home));
    action->setStatusTip(tr("Go back to the initial view"));
    connect(action, SIGNAL(triggered()), this, SLOT(goHome()));
    addAction(action);
    */

    /*
    QIcon homeIcon = QtIconLoader::icon("go-home", QIcon(":/images/go-home.png"));
    QPixmap homePixmap = homeIcon.pixmap(QSize(24,24));
    QToolButton *homeLabel = new QToolButton(this);
    homeLabel->setIcon(QIcon(homePixmap));
    // homeLabel->setStyleSheet("QPushButton {padding: 8px 5px 8px 10px}");
    homeLabel->setAutoRaise(true);
    QToolBar::addWidget(homeLabel);
    */

    /*
    layout->addWidget(homeLabel);
    setLayout(layout);
    */
}

void BreadcrumbWidget::addWidget(QWidget *widget) {
    /*
    BreadcrumbLabel *label = new BreadcrumbLabel(this);
    label->setText("<a href=\"" + widget->windowTitle()
                                       + "\" style=\"color:palette(text); text-decoration:none\">"
                                       + widget->windowTitle() + "</a>");
    label->setStyleSheet("QLabel {padding: 0 10px 0 15px}");
    label->setMargin(0);
    // label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    widgetLabels.insert(widget, label);
    */
    // QToolButton *item = new QToolButton(this);
    // item->setText(widget->windowTitle());
    QAction *action = QToolBar::addAction(widget->windowTitle());
    // connect(action, SIGNAL(triggered()), widget, SLOT(appear()));

}

void BreadcrumbWidget::removeWidget(QWidget *widget) {
    QLabel *label = widgetLabels.value(widget);
    widgetLabels.remove(widget);
    layout()->removeWidget(widget);
    delete label;
}

void BreadcrumbWidget::clear() {
    qDeleteAll(widgetLabels.values());
    widgetLabels.clear();
    foreach (QAction *action, actions()) {
        // removeAction(action);
    }
}
