/*
    SPDX-FileCopyrightText: 2010 Simon Andreas Eugster <simon.eu@gmail.com>
    This file is part of kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "histogramgenerator.h"

#include "klocalizedstring.h"
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <cmath>

HistogramGenerator::HistogramGenerator() = default;

QImage HistogramGenerator::calculateHistogram(const QSize &paradeSize, qreal scalingFactor, const QImage &image, const int &components, ITURec rec,
                                              bool unscaled, bool logScale, uint accelFactor, const QPalette &palette) const
{
    if (paradeSize.height() <= 0 || paradeSize.width() <= 0 || image.width() <= 0 || image.height() <= 0) {
        return QImage();
    }

    bool drawY = (components & HistogramGenerator::ComponentY) != 0;
    bool drawR = (components & HistogramGenerator::ComponentR) != 0;
    bool drawG = (components & HistogramGenerator::ComponentG) != 0;
    bool drawB = (components & HistogramGenerator::ComponentB) != 0;
    bool drawSum = (components & HistogramGenerator::ComponentSum) != 0;

    int r[256], g[256], b[256], y[256], s[766];
    // Initialize the values to zero
    std::fill(r, r + 256, 0);
    std::fill(g, g + 256, 0);
    std::fill(b, b + 256, 0);
    std::fill(y, y + 256, 0);
    std::fill(s, s + 766, 0);

    const int ww = paradeSize.width();
    const int wh = paradeSize.height();

    // Read the stats from the input image
    for (int Y = 0; Y < image.height(); ++Y) {
        for (int X = 0; X < image.width(); X += accelFactor) {

            QRgb col = image.pixel(X, Y);
            r[qRed(col)]++;
            g[qGreen(col)]++;
            b[qBlue(col)]++;

            if (drawY) {
                // Use if branch to avoid expensive multiplication if Y disabled
                if (rec == ITURec::Rec_601) {
                    y[int(REC_601_R * qRed(col) + REC_601_G * qGreen(col) + REC_601_B * qBlue(col))]++;
                } else {
                    y[int(REC_709_R * qRed(col) + REC_709_G * qGreen(col) + REC_709_B * qBlue(col))]++;
                }
            }

            if (drawSum) {
                // Use an if branch here because the sum takes more operations than rgb
                s[qRed(col)]++;
                s[qGreen(col)]++;
                s[qBlue(col)]++;
            }
        }
    }

    const int nParts = (drawY ? 1 : 0) + (drawR ? 1 : 0) + (drawG ? 1 : 0) + (drawB ? 1 : 0) + (drawSum ? 1 : 0);
    if (nParts == 0) {
        // Nothing to draw
        return QImage();
    }

    // Distance for text
    const int d = 20;

    // Height of a single histogram box without text
    const int partH = (wh - nParts * d) / nParts;

    // Total number of bytes of the image
    const int byteCount = int(image.sizeInBytes());

    // Factor for scaling the measured value to the histogram.
    // This factor is used for linear scaling and does not depend
    // on the measured histogram values. Very large values,
    // e.g. in an image with a lot of white, are clipped.
    // Otherwise, the relatively low height of the histogram
    // would show all other values close to 0 when one bin is very high.
    float scaling = 0;
    int div = byteCount >> 7;
    if (div > 0) {
        scaling = partH / float(byteCount >> 7);
    }
    const int dist = 40;

    QImage histogram(paradeSize * scalingFactor, QImage::Format_ARGB32);
    histogram.setDevicePixelRatio(scalingFactor);
    QPainter davinci;
    bool ok = davinci.begin(&histogram);
    if (!ok) {
        qDebug() << "Could not initialise QPainter for Histogram.";
        return histogram;
    }
    davinci.setPen(palette.text().color());
    histogram.fill(qRgba(0, 0, 0, 0));

    QColor neutralColor = palette.text().color();
    QColor redColor = Qt::red;
    QColor greenColor = Qt::green;
    QColor blueColor = QColor(51, 51, 255);
    QColor backgroundColor = palette.base().color();

    int wy = 0; // Drawing position

    if (drawY) {
        drawComponentFull(&davinci, y, scaling, QRect(0, wy, ww, partH + dist), neutralColor, backgroundColor, dist, unscaled, logScale, 256);
        wy += partH + d;
    }

    if (drawSum) {
        drawComponentFull(&davinci, s, scaling / 3, QRect(0, wy, ww, partH + dist), neutralColor, backgroundColor, dist, unscaled, logScale, 256);
        wy += partH + d;
    }

    if (drawR) {
        drawComponentFull(&davinci, r, scaling, QRect(0, wy, ww, partH + dist), redColor, backgroundColor, dist, unscaled, logScale, 256);
        wy += partH + d;
    }

    if (drawG) {
        drawComponentFull(&davinci, g, scaling, QRect(0, wy, ww, partH + dist), greenColor, backgroundColor, dist, unscaled, logScale, 256);
        wy += partH + d;
    }

    if (drawB) {
        drawComponentFull(&davinci, b, scaling, QRect(0, wy, ww, partH + dist), blueColor, backgroundColor, dist, unscaled, logScale, 256);
    }

    return histogram;
}

QImage HistogramGenerator::drawComponent(const int *y, const QSize &size, const float &scaling, const QColor &color, const QColor &backgroundColor,
                                         bool unscaled, bool logScale, int max)
{
    QImage component(max, size.height(), QImage::Format_ARGB32);
    component.fill(backgroundColor);
    Q_ASSERT(scaling != INFINITY);

    const int partH = size.height();

    const int maxBinSize = *std::max_element(&y[0], &y[max]);
    const float logScaling = float(size.height()) / log10f(float(maxBinSize + 1));

    for (int x = 0; x < max; ++x) {

        // Calculate the height of the curve at position x
        int partY;
        if (logScale) {
            partY = int(logScaling * log10f(float(y[x] + 1)));
        } else {
            partY = int(scaling * y[x]);
        }

        // Invert the y axis
        if (partY > partH - 1) {
            partY = partH - 1;
        }
        partY = partH - 1 - partY;

        for (int k = partH - 1; k >= partY; --k) {
            component.setPixel(x, k, color.rgba());
        }
    }
    if (unscaled && size.width() >= component.width()) {
        return component;
    }
    return component.scaled(size, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

void HistogramGenerator::drawComponentFull(QPainter *davinci, const int *y, const float &scaling, const QRect &rect, const QColor &color,
                                           const QColor &backgroundColor, int textSpace, bool unscaled, bool logScale, int max)
{
    QImage component = drawComponent(y, rect.size() - QSize(0, textSpace), scaling, color, backgroundColor, unscaled, logScale, max);
    davinci->drawImage(rect.topLeft(), component);

    int min = 0;
    for (int x = 0; x < max; ++x) {
        min = x;
        if (y[x] > 0) {
            break;
        }
    }
    int maxVal = max - 1;
    for (int x = max - 1; x >= 0; --x) {
        maxVal = x;
        if (y[x] > 0) {
            break;
        }
    }

    const int textY = rect.bottom() - textSpace + 15;
    const int dist = 40;
    const int cw = component.width();

    davinci->drawText(0, textY, i18n("min"));
    davinci->drawText(dist, textY, QString::number(min, 'f', 0));

    davinci->drawText(cw - dist - 30, textY, i18n("max"));
    davinci->drawText(cw - 30, textY, QString::number(maxVal, 'f', 0));
}
