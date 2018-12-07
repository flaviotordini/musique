#include "toolbarmenu.h"
#include "mainwindow.h"

ToolbarMenu::ToolbarMenu(QWidget *parent) : QMenu(parent) {
    MainWindow *w = MainWindow::instance();
    addAction(w->getAction("stopafterthis"));
    addSeparator();
    addAction(w->getAction("finetune"));
    addAction(w->getAction("chooseFolder"));
    addSeparator();
    addAction(w->getAction("lastFmLogout"));
#ifndef APP_MAC
    addSeparator();
    addAction(w->getAction("toggleMenu"));
#endif
    addSeparator();
    addMenu(w->getMenu("help"));
}

void ToolbarMenu::showEvent(QShowEvent *e) {
    Q_UNUSED(e);
    QAction *a = MainWindow::instance()->getAction("stopafterthis");
    QStyleOptionMenuItem option;
    initStyleOption(&option, a);
    int leftMargin = option.maxIconWidth;
#ifndef APP_MAC
    // On Win & Linux the value is wrong
    leftMargin *= 1.5;
#endif
    emit leftMarginChanged(leftMargin);
}
