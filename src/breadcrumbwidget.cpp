#include "breadcrumbwidget.h"
#include "iconloader/qticonloader.h"

BreadcrumbWidget::BreadcrumbWidget(QWidget *parent) : QToolBar(parent) {

    QPalette p = palette();
    p.setBrush(QPalette::Window, Qt::black);
    p.setBrush(QPalette::Text, Qt::white);
    p.setBrush(QPalette::WindowText, Qt::white);
    p.setBrush(QPalette::ButtonText, Qt::white);
    setPalette(p);

    backAction = new QAction(
            QtIconLoader::icon("go-previous"),
            tr("&Back"), this);
    QKeySequence keySequence(Qt::ALT + Qt::Key_Left);
    backAction->setShortcut(keySequence);
    backAction->setStatusTip(tr("Go back") + " (" +
                             keySequence.toString(QKeySequence::NativeText) + ")");
    connect(backAction, SIGNAL(triggered()), SIGNAL(goneBack()));
    addAction(backAction);

    setStyleSheet("QToolButton { color:white } QToolBar { border:0 }");
}

void BreadcrumbWidget::addItem(QString title) {

    QAction *action = addAction(title);
    action->setEnabled(false);

    /*
    // all actions enabled but the last one
    foreach (QAction *a, actions())
        a->setEnabled(a != action);
        */

}

void BreadcrumbWidget::goBack() {

    if (actions().size() > 1) {
        QAction *action = actions().last();
        if (action) {
            removeAction(action);
            delete action;
        }
    }

    /*
    // all actions enabled but the last one
    QAction *lastAction = actions().last();
    foreach (QAction *a, actions())
        a->setEnabled(a != lastAction);
        */
}

void BreadcrumbWidget::clear() {
    // remove all but the backAction
    foreach (QAction *action, actions()) {
        if (action != backAction) {
            removeAction(action);
            delete action;
        }
    }
}
