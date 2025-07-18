/*
    SPDX-FileCopyrightText: 2010 Simon Andreas Eugster <simon.eu@gmail.com>
    This file is part of kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "colorconstants.h"
#include <QObject>
#include <QPalette>

class QImage;
class QSize;

class WaveformGenerator : public QObject
{
    Q_OBJECT

public:
    enum PaintMode { PaintMode_Green, PaintMode_Yellow, PaintMode_White };

    WaveformGenerator();
    ~WaveformGenerator() override;

    QImage calculateWaveform(const QSize &waveformSize, qreal scalingFactor, const QImage &image, const WaveformGenerator::PaintMode paintMode, bool drawAxis,
                             const ITURec rec, uint accelFactor = 1);
    static const uchar distBorder;
};
