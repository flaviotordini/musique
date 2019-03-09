/* $BEGIN_LICENSE

This file is part of Minitube.
Copyright 2009, Flavio Tordini <flavio.tordini@gmail.com>

Minitube is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Minitube is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Minitube.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#include "fontutils.h"
#ifdef APP_MAC
#include "macutils.h"
#endif

namespace {

QFont createFont(bool isBold, double ratio) {
    QFont font = qApp->font();
    font.setPointSize(font.pointSize() * ratio);
    font.setBold(isBold);
    if (ratio < 1.0) {
        const int minPointSize = 8;
        if (font.pointSize() < minPointSize) {
            qDebug() << "Fixing font size to" << minPointSize;
            font.setPointSize(minPointSize);
        }
    }
    return font;
}

} // namespace

const QFont &FontUtils::small() {
    static const QFont font = createFont(false, .9);
    return font;
}

const QFont &FontUtils::smallBold() {
    static const QFont font = createFont(true, .9);
    return font;
}

const QFont &FontUtils::medium() {
    static const QFont font = createFont(false, 1.15);
    return font;
}

const QFont &FontUtils::mediumBold() {
    static const QFont font = createFont(true, 1.15);
    return font;
}

const QFont &FontUtils::big() {
    static const QFont font = createFont(false, 1.5);
    return font;
}

const QFont &FontUtils::bigBold() {
    static const QFont font = createFont(true, 1.5);
    return font;
}

QFont FontUtils::light(int pointSize) {
#ifdef APP_MAC
    QVariant v = mac::lightFont(pointSize);
    if (!v.isNull()) return qvariant_cast<QFont>(v);
#endif
    QFont f = qApp->font();
#ifdef APP_WIN
    f.setFamily(QStringLiteral("Segoe UI Light"));
#endif
    f.setPointSize(pointSize);
    f.setStyleName(QStringLiteral("Light"));
    f.setWeight(QFont::Light);
    return f;
}
