#include <QPainter>
#include <QPaintEvent>
#include <QList>
#include <QtGui>

#include "thblackbar.h"
#include "../fontutils.h"

/* ============================================================================
 *  PRIVATE Class
 */
class THBlackBar::Private {
public:
    QList<QAction *> actionList;
    QAction *checkedAction;
    QAction *hoveredAction;
};

/* ============================================================================
 *  PUBLIC Constructor/Destructors
 */
THBlackBar::THBlackBar (QWidget *parent)
    : QWidget(parent), d(new THBlackBar::Private)
{
    // Setup Widget Options
    setMouseTracking(true);

    // Setup Members
    d->hoveredAction = NULL;
    d->checkedAction = NULL;
}

THBlackBar::~THBlackBar() {
    delete d;
}

/* ============================================================================
 *  PUBLIC Methods
 */
QAction *THBlackBar::addAction (QAction *action) {
    action->setCheckable(true);
    d->actionList.append(action);
    return(action);
}

QAction *THBlackBar::addAction(const QString& text) {
    QAction *action = new QAction(text, this);
    action->setCheckable(true);
    d->actionList.append(action);
    return(action);
}

void THBlackBar::setCheckedAction(int index) {
    if (d->checkedAction)
        d->checkedAction->setChecked(false);
    if (index < 0) {
        d->checkedAction = 0;
        return;
    }
    d->checkedAction = d->actionList.at(index);
    d->checkedAction->setChecked(true);
    update();
}

void THBlackBar::setCheckedAction(QAction *action) {
    if (d->checkedAction == action) return;
    if (d->checkedAction)
        d->checkedAction->setChecked(false);
    d->checkedAction = action;
    d->checkedAction->setChecked(true);
    update();
}

QSize THBlackBar::minimumSizeHint (void) const {
    int itemsWidth = calculateButtonWidth() * d->actionList.size() * 1.2;
    return(QSize(itemsWidth, QFontMetrics(font()).height() * 1.9));
}

/* ============================================================================
 *  PROTECTED Methods
 */
void THBlackBar::paintEvent (QPaintEvent *event) {
    int height = event->rect().height();
    int width = event->rect().width();

    QPainter p(this);

    // Calculate Buttons Size & Location
    int buttonWidth = width / d->actionList.size();

    // Draw Buttons
    QRect rect(0, 0, buttonWidth, height);
    int actionCount = d->actionList.size();
    for (int i = 0; i < actionCount; i++) {
        QAction *action = d->actionList.at(i);

        if (i + 1 == actionCount) {
            rect.setWidth(width - buttonWidth * (actionCount-1));
            drawButton(&p, rect, action);
        } else {
            drawButton(&p, rect, action);
            rect.moveLeft(rect.x() + rect.width());
        }

    }
    /*
    foreach (QAction *action, d->actionList) {
        drawButton(&p, rect, action);
        rect.moveLeft(rect.x() + rect.width());
    }*/

}

void THBlackBar::mouseMoveEvent (QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);

    QAction *action = hoveredAction(event->pos());

    if (action == NULL && d->hoveredAction != NULL) {
        // d->hoveredAction->hover(false);
        d->hoveredAction = NULL;
        update();
    } else if (action != NULL) {
        d->hoveredAction = action;
        action->hover();
        update();

        // status tip
        QMainWindow* mainWindow = dynamic_cast<QMainWindow*>(window());
        if (mainWindow) mainWindow->statusBar()->showMessage(action->statusTip());
    }
}

void THBlackBar::mousePressEvent (QMouseEvent *event) {
    QWidget::mousePressEvent(event);

    if (d->hoveredAction != NULL) {

        if (d->checkedAction != NULL) {
            // already checked
            // if (d->checkedAction == d->hoveredAction) return;
            d->checkedAction->setChecked(false);
        }

        d->checkedAction = d->hoveredAction;
        d->hoveredAction->setChecked(true);
        d->hoveredAction->trigger();

        // this will also call update()
        setCheckedAction(d->actionList.indexOf(d->hoveredAction));
    }
}

void THBlackBar::leaveEvent(QEvent * /* event */) {
    // status tip
    QMainWindow* mainWindow = dynamic_cast<QMainWindow*>(window());
    if (mainWindow) mainWindow->statusBar()->clearMessage();
}

QAction *THBlackBar::hoveredAction (const QPoint& pos) const {
    if (pos.y() <= 0 || pos.y() >= height())
        return(NULL);

    int buttonWidth = width() / d->actionList.size();;
    int buttonsWidth = width();
    int buttonsX = 0;

    if (pos.x() <= buttonsX || pos.x() >= (buttonsX + buttonsWidth))
        return(NULL);

    int buttonIndex = (pos.x() - buttonsX) / buttonWidth;

    if (buttonIndex >= d->actionList.size())
        return(NULL);
    return(d->actionList[buttonIndex]);
}

int THBlackBar::calculateButtonWidth (void) const {
    QFont smallerBoldFont = FontUtils::smallBold();
    QFontMetrics fontMetrics(smallerBoldFont);
    int tmpItemWidth, itemWidth = 0;
    foreach (QAction *action, d->actionList) {
        tmpItemWidth = fontMetrics.width(action->text());
        if (itemWidth < tmpItemWidth) itemWidth = tmpItemWidth;
    }
    return itemWidth;
}


/* ============================================================================
 *  PRIVATE Methods
 */
void THBlackBar::drawUnselectedButton (	QPainter *painter,
                                        const QRect& rect,
                                        const QAction *action)
{
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, rect.height() / 2));
    linearGrad.setColorAt(0, QColor(0x8e, 0x8e, 0x8e));
    linearGrad.setColorAt(1, QColor(0x5c, 0x5c, 0x5c));
    drawButton(painter, rect, linearGrad, QColor(0x41, 0x41, 0x41), action);
}

void THBlackBar::drawSelectedButton (	QPainter *painter,
                                        const QRect& rect,
                                        const QAction *action)
{
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, rect.height() / 2));
    linearGrad.setColorAt(0, QColor(0x6d, 0x6d, 0x6d));
    linearGrad.setColorAt(1, QColor(0x25, 0x25, 0x25));
    drawButton(painter, rect, linearGrad, QColor(0x00, 0x00, 0x00), action);
}

void THBlackBar::drawButton (	QPainter *painter,
                                const QRect& rect,
                                const QAction *action)
{
    if (action->isChecked())
        drawSelectedButton(painter, rect, action);
    else
        drawUnselectedButton(painter, rect, action);
}

void THBlackBar::drawButton (	QPainter *painter,
                                const QRect& rect,
                                const QLinearGradient& gradient,
                                const QColor& color,
                                const QAction *action)
{
    painter->save();

    int height = rect.height();
    int width = rect.width();
    int mh = (height / 2);

    painter->translate(rect.x(), rect.y());
    painter->setPen(Qt::black);

    painter->fillRect(0, 0, width, mh, QBrush(gradient));
    painter->fillRect(0, mh, width, mh, color);
#ifdef APP_MAC
    painter->drawRect(-1, -1, width+1, height);
#else
    painter->drawRect(0, 0, width, height-1);
#endif
    QFont smallerBoldFont = FontUtils::smallBold();
    painter->setFont(smallerBoldFont);

    // text shadow
    painter->setPen(QColor(0, 0, 0, 128));
    painter->drawText(0, -1, width, height, Qt::AlignCenter, action->text());

    painter->setPen(QPen(QColor(0xff, 0xff, 0xff), 1));
    painter->drawText(0, 0, width, height, Qt::AlignCenter, action->text());

    painter->restore();
}

