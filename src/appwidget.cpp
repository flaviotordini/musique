#include "appwidget.h"
#include "constants.h"
#include "http.h"
#include "httputils.h"

#if defined(APP_EXTRA) && !defined(APP_MAC_STORE)
#include "updatedialog.h"
#endif

AppsWidget::AppsWidget(QWidget *parent) : QWidget(parent) {
    const int padding = 30;

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(padding, padding, padding, padding);
    layout->setSpacing(padding * 2);
    layout->setAlignment(Qt::AlignCenter);
}

void AppsWidget::add(QString name, QString unixName, QString ext) {
    auto w = new AppWidget(name, unixName, ext, this);
    layout()->addWidget(w);
}

void AppsWidget::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}

AppWidget::AppWidget(const QString &name,
                     const QString &unixName,
                     const QString &ext,
                     QWidget *parent)
    : QWidget(parent), name(name), unixName(unixName), downloadButton(nullptr) {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignHCenter);

    iconLabel = new QLabel();
    iconLabel->setMinimumHeight(128);
    auto loadIconPixmap = [this, unixName] {
        QString twoX;
        if (devicePixelRatio() > 1.0) twoX = '@' + QString::number(devicePixelRatio()) + 'x';
        QString url = "https://" + QLatin1String(Constants::ORG_DOMAIN) + "/files/products/" +
                      unixName + twoX + ".png";
        auto reply = HttpUtils::cached().get(url);
        connect(reply, &HttpReply::data, this, [this](auto bytes) {
            QPixmap pixmap;
            pixmap.loadFromData(bytes, "PNG");
            pixmap.setDevicePixelRatio(devicePixelRatio());
            iconLabel->setPixmap(pixmap);
        });
    };
    connect(window()->windowHandle(), &QWindow::screenChanged, this, loadIconPixmap);
    loadIconPixmap();
    layout->addWidget(iconLabel);

    QLabel *appTitle = new QLabel(name);
    appTitle->setAlignment(Qt::AlignHCenter);
    layout->addWidget(appTitle);

#if defined(APP_EXTRA) && !defined(APP_MAC_STORE)
    downloadButton = new QPushButton(tr("Download"));
    QSizePolicy sp = downloadButton->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Fixed);
    sp.setRetainSizeWhenHidden(true);
    downloadButton->setSizePolicy(sp);
    connect(downloadButton, &QAbstractButton::clicked, this, [this, name, unixName, ext] {
        QString url = QLatin1String("https://") + Constants::ORG_DOMAIN + "/files/" + unixName +
                      "/" + unixName + "." + ext;
        auto dialog = new UpdateDialog(iconLabel->pixmap(), name, QString(), url, this);
        dialog->downloadUpdate();
        dialog->show();
    });
    layout->addWidget(downloadButton, Qt::AlignHCenter);
    layout->setAlignment(downloadButton, Qt::AlignHCenter);
    downloadButton->hide();
#endif

    setCursor(Qt::PointingHandCursor);
}

void AppWidget::enterEvent(QEnterEvent *e) {
    Q_UNUSED(e);
    if (downloadButton) downloadButton->show();
}

void AppWidget::leaveEvent(QEvent *e) {
    Q_UNUSED(e);
    if (downloadButton) downloadButton->hide();
}

void AppWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QString webPage = QLatin1String("https://") + Constants::ORG_DOMAIN + "/" + unixName;
        QDesktopServices::openUrl(webPage);
    }
}
