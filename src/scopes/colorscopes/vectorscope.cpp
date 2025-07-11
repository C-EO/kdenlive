/*
    SPDX-FileCopyrightText: 2010 Simon Andreas Eugster <simon.eu@gmail.com>
    This file is part of kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "vectorscope.h"
#include "colorplaneexport.h"
#include "utils/colortools.h"
#include "vectorscopegenerator.h"

#include "core.h"
#include "kdenlive_debug.h"
#include "klocalizedstring.h"
#include <KConfigGroup>
#include <KSharedConfig>
#include <QAction>
#include <QActionGroup>
#include <QElapsedTimer>
#include <QPainter>
#include <cmath>
const double P75 = .75;

const QPointF YUV_R(-.147, .615);
const QPointF YUV_G(-.289, -.515);
const QPointF YUV_B(.437, -.100);
const QPointF YUV_Cy(.147, -.615);
const QPointF YUV_Mg(.289, .515);
const QPointF YUV_Yl(-.437, .100);

const QPointF YPbPr_R(-.169, .5);
const QPointF YPbPr_G(-.331, -.419);
const QPointF YPbPr_B(.5, -.081);
const QPointF YPbPr_Cy(.169, -.5);
const QPointF YPbPr_Mg(.331, .419);
const QPointF YPbPr_Yl(-.5, .081);

Vectorscope::Vectorscope(QWidget *parent)
    : AbstractGfxScopeWidget(true, parent)

{
    // overwrite custom scopes palette from AbstractScopeWidget with global app palette to respect users theme preference
    setPalette(QPalette());

    m_ui = new Ui::Vectorscope_UI();
    m_ui->setupUi(this);

    m_colorTools = new ColorTools();
    m_vectorscopeGenerator = new VectorscopeGenerator();

    m_ui->paintMode->addItem(i18n("Green 2"), QVariant(VectorscopeGenerator::PaintMode_Green2));
    m_ui->paintMode->addItem(i18n("Green"), QVariant(VectorscopeGenerator::PaintMode_Green));
    m_ui->paintMode->addItem(i18n("Black"), QVariant(VectorscopeGenerator::PaintMode_Black));
    m_ui->paintMode->addItem(i18n("Modified YUV (Chroma)"), QVariant(VectorscopeGenerator::PaintMode_Chroma));
    m_ui->paintMode->addItem(i18n("YUV"), QVariant(VectorscopeGenerator::PaintMode_YUV));
    m_ui->paintMode->addItem(i18n("Original Color"), QVariant(VectorscopeGenerator::PaintMode_Original));

    m_ui->backgroundMode->addItem(i18n("Black"), QVariant(BG_DARK));
    m_ui->backgroundMode->addItem(i18n("YUV"), QVariant(BG_YUV));
    m_ui->backgroundMode->addItem(i18n("Modified YUV (Chroma)"), QVariant(BG_CHROMA));
    m_ui->backgroundMode->addItem(i18n("YPbPr"), QVariant(BG_YPbPr));

    m_ui->sliderGain->setMinimum(0);
    m_ui->sliderGain->setMaximum(40);

    connect(m_ui->backgroundMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Vectorscope::slotBackgroundChanged);
    connect(m_ui->sliderGain, &QAbstractSlider::valueChanged, this, &Vectorscope::slotGainChanged);
    connect(m_ui->paintMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Vectorscope::forceUpdateScope);
    connect(this, &Vectorscope::signalMousePositionChanged, this, &Vectorscope::forceUpdateHUD);
    connect(pCore.get(), &Core::updatePalette, this, [this]() { forceUpdate(true); });
    m_ui->sliderGain->setValue(0);

    ///// Build context menu /////

    m_menu->addSeparator()->setText(i18n("Tools"));

    m_aExportBackground = new QAction(i18n("Export background"), this);
    m_menu->addAction(m_aExportBackground);
    connect(m_aExportBackground, &QAction::triggered, this, &Vectorscope::slotExportBackground);

    m_menu->addSeparator()->setText(i18n("Drawing options"));

    m_a75PBox = new QAction(i18n("75% box"), this);
    m_a75PBox->setCheckable(true);
    m_menu->addAction(m_a75PBox);
    connect(m_a75PBox, &QAction::changed, this, &Vectorscope::forceUpdateBackground);

    m_aAxisEnabled = new QAction(i18n("Draw axis"), this);
    m_aAxisEnabled->setCheckable(true);
    m_menu->addAction(m_aAxisEnabled);
    connect(m_aAxisEnabled, &QAction::changed, this, &Vectorscope::forceUpdateBackground);

    m_aIQLines = new QAction(i18n("Draw I/Q lines"), this);
    m_aIQLines->setCheckable(true);
    m_menu->addAction(m_aIQLines);
    connect(m_aIQLines, &QAction::changed, this, &Vectorscope::forceUpdateBackground);

    m_menu->addSeparator()->setText(i18n("Color Space"));
    m_aColorSpace_YPbPr = new QAction(i18n("YPbPr"), this);
    m_aColorSpace_YPbPr->setCheckable(true);
    m_aColorSpace_YUV = new QAction(i18n("YUV"), this);
    m_aColorSpace_YUV->setCheckable(true);
    m_agColorSpace = new QActionGroup(this);
    m_agColorSpace->addAction(m_aColorSpace_YPbPr);
    m_agColorSpace->addAction(m_aColorSpace_YUV);
    m_menu->addAction(m_aColorSpace_YPbPr);
    m_menu->addAction(m_aColorSpace_YUV);
    connect(m_aColorSpace_YPbPr, &QAction::toggled, this, &Vectorscope::slotColorSpaceChanged);
    connect(m_aColorSpace_YUV, &QAction::toggled, this, &Vectorscope::slotColorSpaceChanged);

    // To make the 1.0x text show
    slotGainChanged(m_ui->sliderGain->value());

    init();
}

Vectorscope::~Vectorscope()
{
    writeConfig();

    delete m_colorTools;
    delete m_vectorscopeGenerator;

    delete m_aColorSpace_YPbPr;
    delete m_aColorSpace_YUV;
    delete m_aExportBackground;
    delete m_aAxisEnabled;
    delete m_a75PBox;
    delete m_agColorSpace;
    delete m_ui;
}

QString Vectorscope::widgetName() const
{
    return QStringLiteral("Vectorscope");
}

void Vectorscope::readConfig()
{
    AbstractGfxScopeWidget::readConfig();

    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup scopeConfig(config, configName());
    m_a75PBox->setChecked(scopeConfig.readEntry("75PBox", false));
    m_aAxisEnabled->setChecked(scopeConfig.readEntry("axis", false));
    m_aIQLines->setChecked(scopeConfig.readEntry("iqlines", false));
    m_ui->backgroundMode->setCurrentIndex(scopeConfig.readEntry("backgroundmode").toInt());
    m_ui->paintMode->setCurrentIndex(scopeConfig.readEntry("paintmode").toInt());
    m_ui->sliderGain->setValue(scopeConfig.readEntry("gain", 1));
    m_aColorSpace_YPbPr->setChecked(scopeConfig.readEntry("colorspace_ypbpr", false));
    m_aColorSpace_YUV->setChecked(!m_aColorSpace_YPbPr->isChecked());
}

void Vectorscope::writeConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup scopeConfig(config, configName());
    scopeConfig.writeEntry("75PBox", m_a75PBox->isChecked());
    scopeConfig.writeEntry("axis", m_aAxisEnabled->isChecked());
    scopeConfig.writeEntry("iqlines", m_aIQLines->isChecked());
    scopeConfig.writeEntry("backgroundmode", m_ui->backgroundMode->currentIndex());
    scopeConfig.writeEntry("paintmode", m_ui->paintMode->currentIndex());
    scopeConfig.writeEntry("gain", m_ui->sliderGain->value());
    scopeConfig.writeEntry("colorspace_ypbpr", m_aColorSpace_YPbPr->isChecked());
    scopeConfig.sync();
}

QRect Vectorscope::scopeRect()
{
    // Distance from top/left/right
    int border = 6;

    // We want to paint below the controls area. The line is the lowest element.
    QPoint topleft(border, m_ui->verticalSpacer->geometry().y() + border);
    QPoint bottomright(m_ui->horizontalSpacer->geometry().right() - border, this->size().height() - border);

    m_visibleRect = QRect(topleft, bottomright);

    QRect scopeRect(topleft, bottomright);

    // Circle Width: min of width and height
    m_cw = (scopeRect.height() < scopeRect.width()) ? scopeRect.height() : scopeRect.width();
    scopeRect.setWidth(m_cw);
    scopeRect.setHeight(m_cw);

    m_centerPoint = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), QPointF(0, 0));
    m_pR75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YUV_R);
    m_pG75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YUV_G);
    m_pB75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YUV_B);
    m_pCy75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YUV_Cy);
    m_pMg75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YUV_Mg);
    m_pYl75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YUV_Yl);
    m_qR75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YPbPr_R);
    m_qG75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YPbPr_G);
    m_qB75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YPbPr_B);
    m_qCy75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YPbPr_Cy);
    m_qMg75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YPbPr_Mg);
    m_qYl75 = m_vectorscopeGenerator->mapToCircle(scopeRect.size(), P75 * VectorscopeGenerator::scaling * YPbPr_Yl);

    return scopeRect;
}

bool Vectorscope::isHUDDependingOnInput() const
{
    return false;
}
bool Vectorscope::isScopeDependingOnInput() const
{
    return true;
}
bool Vectorscope::isBackgroundDependingOnInput() const
{
    return false;
}

QImage Vectorscope::renderHUD(uint)
{

    QImage hud;
    QLocale locale; // Used for UI → OK
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    if (m_mouseWithinWidget) {
        // Mouse moved: Draw a circle over the scope

        qreal dpr = devicePixelRatioF();
        hud = QImage(m_visibleRect.size() * dpr, QImage::Format_ARGB32);
        hud.setDevicePixelRatio(dpr);
        hud.fill(qRgba(0, 0, 0, 0));

        QPainter davinci;
        bool ok = davinci.begin(&hud);
        if (!ok) {
            qDebug() << "Could not initialise QPainter for Vectorscope HUD.";
            return hud;
        }
        QPoint widgetCenterPoint = m_scopeRect.topLeft() + m_centerPoint;

        int dx = -widgetCenterPoint.x() + m_mousePos.x();
        int dy = widgetCenterPoint.y() - m_mousePos.y();

        QPoint reference = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), QPointF(1, 0));

        float r = sqrtf(dx * dx + dy * dy);
        float percent = 100.f * r / float(VectorscopeGenerator::scaling) / m_gain / (reference.x() - widgetCenterPoint.x());

        QPen penHighlight(QBrush(palette().highlight().color()), penLight.width(), Qt::SolidLine);

        switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
        case BG_DARK:
            davinci.setPen(penHighlight);
            break;
        default:
            if (r > m_cw / 2.0f) {
                davinci.setPen(penHighlight);
            } else {
                davinci.setPen(penDark);
            }
            break;
        }
        davinci.setRenderHint(QPainter::Antialiasing, true);
        davinci.drawEllipse(m_centerPoint, int(r), int(r));
        davinci.setRenderHint(QPainter::Antialiasing, false);
        davinci.setPen(penHighlight);
        davinci.drawText(QPoint(m_scopeRect.width() - 40, m_scopeRect.height()), i18n("%1%", locale.toString(percent, 'f', 0)));

        float angle = float(copysignf(std::acos(dx / r) * 180.f / float(M_PI), dy));
        davinci.drawText(QPoint(10, m_scopeRect.height()), i18n("%1°", locale.toString(angle, 'f', 1)));

    } else {
        hud = QImage(0, 0, QImage::Format_ARGB32);
    }
    Q_EMIT signalHUDRenderingFinished(0, 1);
    return hud;
}

QImage Vectorscope::renderGfxScope(uint accelerationFactor, const QImage &qimage)
{
    QElapsedTimer timer;
    timer.start();
    QImage scope;

    if (m_cw <= 0) {
        qCDebug(KDENLIVE_LOG) << "Scope size not known yet. Aborting.";
    } else {

        VectorscopeGenerator::ColorSpace colorSpace =
            m_aColorSpace_YPbPr->isChecked() ? VectorscopeGenerator::ColorSpace_YPbPr : VectorscopeGenerator::ColorSpace_YUV;
        VectorscopeGenerator::PaintMode paintMode = VectorscopeGenerator::PaintMode(m_ui->paintMode->itemData(m_ui->paintMode->currentIndex()).toInt());
        qreal dpr = devicePixelRatioF();
        scope = m_vectorscopeGenerator->calculateVectorscope(m_scopeRect.size() * dpr, dpr, qimage, m_gain, paintMode, colorSpace, m_aAxisEnabled->isChecked(),
                                                             accelerationFactor);
    }
    Q_EMIT signalScopeRenderingFinished(uint(timer.elapsed()), accelerationFactor);
    return scope;
}

QImage Vectorscope::renderBackground(uint)
{
    QElapsedTimer timer;
    timer.start();

    qreal scalingFactor = devicePixelRatioF();
    QImage bg(m_visibleRect.size() * scalingFactor, QImage::Format_ARGB32);
    bg.setDevicePixelRatio(scalingFactor);
    bg.fill(qRgba(0, 0, 0, 0));

    // Set up tools
    QPainter davinci;
    bool ok = davinci.begin(&bg);
    if (!ok) {
        qDebug() << "Could not initialise QPainter for Vectorscope background.";
        return bg;
    }
    davinci.setRenderHint(QPainter::Antialiasing, true);

    QPoint vinciPoint;
    QPoint vinciPoint2;

    // Alignment correction
    // ColorTools colorWheel functions and VectorscopeGenerator::mapToCircle are not anti-aliased.
    // As we're drawing the circle border anti-aliased we need to compensate for that as we might spill outside the anti-aliased circle otherwise.
    // When drawing the I/Q lines they could also spill outside the circle in case of fractional HiDPI scaling and potential rounding differences between
    // drawing the circle and the lines. In the following we'll be insetting these drawing calls by 1px and then drawing the circle border with at least 2px
    // width.
    QSize alignmentCorrection(2, 2);
    int alignmentCorrectionOffset = 1;
    QPoint alignmentCorrectionPoint(alignmentCorrectionOffset, alignmentCorrectionOffset);

    // Draw the color plane (if selected)
    QImage colorPlane;
    switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
    case BG_YUV:
        colorPlane = m_colorTools->yuvColorWheel(m_scopeRect.size() - alignmentCorrection, 128, 1 / float(VectorscopeGenerator::scaling), false, true);
        break;
    case BG_CHROMA:
        colorPlane = m_colorTools->yuvColorWheel(m_scopeRect.size() - alignmentCorrection, 255, 1 / float(VectorscopeGenerator::scaling), true, true);
        break;
    case BG_YPbPr:
        colorPlane = m_colorTools->yPbPrColorWheel(m_scopeRect.size() - alignmentCorrection, 128, 1 / float(VectorscopeGenerator::scaling), true);
        break;
    case BG_DARK:
        colorPlane = m_colorTools->FixedColorCircle(m_scopeRect.size() - alignmentCorrection, qRgba(25, 25, 23, 255));
        break;
    }
    davinci.drawImage(alignmentCorrectionOffset, alignmentCorrectionOffset, colorPlane);

    // Draw I/Q lines (from the YIQ color space; Skin tones lie on the I line)
    // Positions are calculated by transforming YIQ:[0 1 0] or YIQ:[0 0 1] to YUV/YPbPr.
    if (m_aIQLines->isChecked()) {
        QPen iqTextLabelPen(QBrush(palette().text().color()), 2, Qt::SolidLine);

        switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
        case BG_DARK:
            davinci.setPen(penLightDots);
            break;
        default:
            davinci.setPen(penDarkDots);
            break;
        }

        if (m_aColorSpace_YUV->isChecked()) {
            vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(-.544, .838));
            vinciPoint2 = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(.544, -.838));
        } else {
            vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(-.675, .737));
            vinciPoint2 = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(.675, -.737));
        }

        davinci.drawLine(vinciPoint + alignmentCorrectionPoint, vinciPoint2 + alignmentCorrectionPoint);
        davinci.setPen(iqTextLabelPen);
        davinci.drawText(vinciPoint - QPoint(11, 5), QStringLiteral("I"));

        switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
        case BG_DARK:
            davinci.setPen(penLightDots);
            break;
        default:
            davinci.setPen(penDarkDots);
            break;
        }

        if (m_aColorSpace_YUV->isChecked()) {
            vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(.838, .544));
            vinciPoint2 = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(-.838, -.544));
        } else {
            vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(.908, .443));
            vinciPoint2 = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size() - alignmentCorrection, QPointF(-.908, -.443));
        }

        davinci.drawLine(vinciPoint + alignmentCorrectionPoint, vinciPoint2 + alignmentCorrectionPoint);
        davinci.setPen(iqTextLabelPen);
        davinci.drawText(vinciPoint - QPoint(-7, 2), QStringLiteral("Q"));
    }

    // Draw the vectorscope circle
    bool isDarkTheme = palette().color(QPalette::Window).lightness() < palette().color(QPalette::WindowText).lightness();
    if (isDarkTheme) {
        davinci.setPen(penThick);
    } else {
        davinci.setPen(QPen(palette().color(QPalette::Dark), penThick.width(), Qt::SolidLine));
    }
    qreal penWidthHalf = davinci.pen().widthF() / 2;
    davinci.drawEllipse(QRectF(penWidthHalf, penWidthHalf, m_cw - davinci.pen().widthF(), m_cw - davinci.pen().widthF()));

    // Draw RGB/CMY points with 100% chroma
    davinci.setPen(penThick);
    if (m_aColorSpace_YUV->isChecked()) {
        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YUV_R);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint - QPoint(20, -10), QStringLiteral("R"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YUV_G);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint - QPoint(20, 0), QStringLiteral("G"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YUV_B);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint + QPoint(15, 10), QStringLiteral("B"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YUV_Cy);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint + QPoint(15, -5), QStringLiteral("Cy"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YUV_Mg);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint + QPoint(15, 10), QStringLiteral("Mg"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YUV_Yl);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint - QPoint(25, 0), QStringLiteral("Yl"));
    } else {
        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YPbPr_R);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint - QPoint(20, -10), QStringLiteral("R"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YPbPr_G);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint - QPoint(20, 0), QStringLiteral("G"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YPbPr_B);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint + QPoint(15, 10), QStringLiteral("B"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YPbPr_Cy);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint + QPoint(15, -5), QStringLiteral("Cy"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YPbPr_Mg);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint + QPoint(15, 10), QStringLiteral("Mg"));

        vinciPoint = m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), VectorscopeGenerator::scaling * YPbPr_Yl);
        davinci.drawEllipse(vinciPoint, 4, 4);
        davinci.drawText(vinciPoint - QPoint(25, 0), QStringLiteral("Yl"));
    }

    // Draw axis
    if (m_aAxisEnabled->isChecked()) {
        switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
        case BG_DARK:
            davinci.setPen(penLight);
            break;
        default:
            davinci.setPen(penDark);
            break;
        }
        davinci.drawLine(m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), QPointF(0, -.9)),
                         m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), QPointF(0, .9)));
        davinci.drawLine(m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), QPointF(-.9, 0)),
                         m_vectorscopeGenerator->mapToCircle(m_scopeRect.size(), QPointF(.9, 0)));
    }

    // Draw center point
    switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
    case BG_CHROMA:
        davinci.setPen(penDark);
        break;
    default:
        davinci.setPen(penThin);
        break;
    }
    davinci.drawEllipse(m_centerPoint, 5, 5);

    // Draw 75% box
    if (m_a75PBox->isChecked()) {
        if (m_aColorSpace_YUV->isChecked()) {
            davinci.drawLine(m_pR75, m_pYl75);
            davinci.drawLine(m_pYl75, m_pG75);
            davinci.drawLine(m_pG75, m_pCy75);
            davinci.drawLine(m_pCy75, m_pB75);
            davinci.drawLine(m_pB75, m_pMg75);
            davinci.drawLine(m_pMg75, m_pR75);
        } else {
            davinci.drawLine(m_qR75, m_qYl75);
            davinci.drawLine(m_qYl75, m_qG75);
            davinci.drawLine(m_qG75, m_qCy75);
            davinci.drawLine(m_qCy75, m_qB75);
            davinci.drawLine(m_qB75, m_qMg75);
            davinci.drawLine(m_qMg75, m_qR75);
        }
    }

    // Draw RGB/CMY points with 75% chroma (for NTSC)
    davinci.setPen(penThin);
    if (m_aColorSpace_YUV->isChecked()) {
        davinci.drawEllipse(m_pR75, 3, 3);
        davinci.drawEllipse(m_pG75, 3, 3);
        davinci.drawEllipse(m_pB75, 3, 3);
        davinci.drawEllipse(m_pCy75, 3, 3);
        davinci.drawEllipse(m_pMg75, 3, 3);
        davinci.drawEllipse(m_pYl75, 3, 3);
    } else {
        davinci.drawEllipse(m_qR75, 3, 3);
        davinci.drawEllipse(m_qG75, 3, 3);
        davinci.drawEllipse(m_qB75, 3, 3);
        davinci.drawEllipse(m_qCy75, 3, 3);
        davinci.drawEllipse(m_qMg75, 3, 3);
        davinci.drawEllipse(m_qYl75, 3, 3);
    }

    // Draw realtime factor (number of skipped pixels)
    if (m_aRealtime->isChecked()) {
        davinci.setPen(penThin);
        davinci.drawText(QPoint(m_scopeRect.width() - 40, m_scopeRect.height() - 15), QVariant(m_accelFactorScope).toString().append(QStringLiteral("x")));
    }

    Q_EMIT signalBackgroundRenderingFinished(uint(timer.elapsed()), 1);
    return bg;
}

///// Slots /////

void Vectorscope::slotGainChanged(int newval)
{
    QLocale locale; // Used for UI → OK
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    m_gain = 1 + newval / 10.f;
    m_ui->lblGain->setText(locale.toString(m_gain, 'f', 1) + QLatin1Char('x'));
    forceUpdateScope();
}

void Vectorscope::slotExportBackground()
{
    QPointer<ColorPlaneExport> colorPlaneExportDialog = new ColorPlaneExport(this);
    colorPlaneExportDialog->exec();
    delete colorPlaneExportDialog;
}

void Vectorscope::slotBackgroundChanged()
{
    // Background changed, switch to a suitable color mode now
    int index;
    switch (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt()) {
    case BG_YUV:
        index = m_ui->paintMode->findData(QVariant(VectorscopeGenerator::PaintMode_Black));
        if (index >= 0) {
            m_ui->paintMode->setCurrentIndex(index);
        }
        break;

    case BG_DARK:
        if (m_ui->paintMode->itemData(m_ui->paintMode->currentIndex()).toInt() == VectorscopeGenerator::PaintMode_Black) {
            index = m_ui->paintMode->findData(QVariant(VectorscopeGenerator::PaintMode_Green2));
            m_ui->paintMode->setCurrentIndex(index);
        }
        break;
    }
    forceUpdateBackground();
}

void Vectorscope::slotColorSpaceChanged()
{
    int index;
    if (m_aColorSpace_YPbPr->isChecked()) {
        if (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt() == BG_YUV) {
            index = m_ui->backgroundMode->findData(QVariant(BG_YPbPr));
            if (index >= 0) {
                m_ui->backgroundMode->setCurrentIndex(index);
            }
        }
    } else {
        if (m_ui->backgroundMode->itemData(m_ui->backgroundMode->currentIndex()).toInt() == BG_YPbPr) {
            index = m_ui->backgroundMode->findData(QVariant(BG_YUV));
            if (index >= 0) {
                m_ui->backgroundMode->setCurrentIndex(index);
            }
        }
    }
    forceUpdate();
}
