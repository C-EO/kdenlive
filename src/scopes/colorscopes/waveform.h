/*
    SPDX-FileCopyrightText: 2010 Simon Andreas Eugster <simon.eu@gmail.com>
    This file is part of kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "abstractgfxscopewidget.h"

#include "ui_waveform_ui.h"

class Waveform_UI;
class WaveformGenerator;

/**
 * @brief Displays the waveform of a frame.
 *
 *  For further explanations of the waveform see the WaveformGenerator class.
*/
class Waveform : public AbstractGfxScopeWidget
{
    Q_OBJECT

public:
    explicit Waveform(QWidget *parent = nullptr);
    ~Waveform() override;

    QString widgetName() const override;

protected:
    void readConfig() override;
    void writeConfig();

private:
    Ui::Waveform_UI *m_ui{nullptr};
    WaveformGenerator *m_waveformGenerator;

    QAction *m_aRec601;
    QAction *m_aRec709;
    QActionGroup *m_agRec;

    int m_textWidth;
    static const int m_paddingBottom;

    QImage m_waveform;

    /// Implemented methods ///
    QRect scopeRect() override;
    QImage renderHUD(uint) override;
    QImage renderGfxScope(uint, const QImage &) override;
    QImage renderBackground(uint) override;
    bool isHUDDependingOnInput() const override;
    bool isScopeDependingOnInput() const override;
    bool isBackgroundDependingOnInput() const override;
};
