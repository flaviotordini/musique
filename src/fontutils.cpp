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
#include <QtCore>

namespace {

QFont createFont(QFont::Weight weight, double sizeScale) {
    QFont font;
    font.setPointSize(font.pointSize() * sizeScale);
#ifdef APP_MAC
    if (weight == QFont::Light) {
        font.setFamily("Helvetica Neue");
    }
#endif
    font.setWeight(weight);
    return font;
}

QFont createFontWithMinSize(QFont::Weight weight, double sizeScale) {
    const int MIN_PIXEL_SIZE = 12;
    QFont font = createFont(weight, sizeScale);
    if (font.pixelSize() < MIN_PIXEL_SIZE)
        font.setPixelSize(MIN_PIXEL_SIZE);
    return font;
}

}

const QFont &FontUtils::small() {
    static const QFont font = createFontWithMinSize(QFont::Normal, .9);
    return font;
}

const QFont &FontUtils::smallBold() {
    static const QFont font = createFontWithMinSize(QFont::Bold, .9);
    return font;
}

const QFont &FontUtils::medium() {
    static const QFont font = createFont(QFont::Normal, 1.1);
    return font;
}

const QFont &FontUtils::mediumBold() {
    static const QFont font = createFont(QFont::Bold, 1.1);
    return font;
}

const QFont &FontUtils::big() {
    static const QFont font = createFont(QFont::Normal, 1.5);
    return font;
}

const QFont &FontUtils::bigBold() {
    static const QFont font = createFont(QFont::Bold, 1.5);
    return font;
}

const QFont &FontUtils::bigger() {
    static const QFont font = createFont(QFont::Light, 2.5);
    return font;
}
