/*
SPDX-FileCopyrightText: 2015 Jean-Baptiste Mardelle <jb@kdenlive.org>
This file is part of Kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "definitions.h"

#include <KMessageWidget>

#include <QString>
#include <QTreeWidget>
#include <QLabel>

#include <mlt++/Mlt.h>

class ClipController;
class QMimeData;
class QTextEdit;
class QComboBox;
class QListWidget;
class QGroupBox;
class QCheckBox;
class QButtonGroup;
class QSpinBox;
class QSortFilterProxyModel;

class ElidedLinkLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ElidedLinkLabel(QWidget *parent = nullptr);
    void setLabelText(const QString &text, const QString &link);
    void updateText(int width);
    int currentWidth() const;

private:
    QString m_text;
    QString m_link;

protected:
    void resizeEvent(QResizeEvent *event) override;
};

class AnalysisTree : public QTreeWidget
{
public:
    explicit AnalysisTree(QWidget *parent = nullptr);

protected:
    QMimeData *mimeData(const QList<QTreeWidgetItem *> &list) const override;
};

/** @class ClipPropertiesController
    @brief This class creates the widgets allowing to edit clip properties
 */
class ClipPropertiesController : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructor.
     * @param clipName The clip's name
     * @param controller The clip's controller
     * @param parent The widget where our infos will be displayed
     */
    explicit ClipPropertiesController(const QString &clipName, ClipController *controller, QWidget *parent);
    ~ClipPropertiesController() override;
    void activatePage(int ix);

public Q_SLOTS:
    void slotReloadProperties();
    void slotFillMeta(QTreeWidget *tree);
    void slotFillAnalysisData();
    void updateStreamInfo(int streamIndex);

private Q_SLOTS:
    void slotColorModified(const QColor &newcolor);
    void slotDurationChanged(int duration);
    void slotEnableForce(int state);
    void slotValueChanged(double);
    void slotDeleteAnalysis();
    void slotSaveAnalysis();
    void slotLoadAnalysis();
    void slotAspectValueChanged(int);
    void slotComboValueChanged();
    void slotValueChanged(int value);
    void slotTextChanged();
    void updateTab(int ix);

private:
    ClipController *m_controller;
    QTabWidget *m_tabWidget;
    ElidedLinkLabel *m_clipLabel;
    QString m_id;
    ClipType::ProducerType m_type;
    /** @brief: the properties of the active producer (can be a proxy) */
    std::shared_ptr<Mlt::Properties> m_properties;
    /** @brief: the properties of the original source producer (cannot be a proxy) */
    std::shared_ptr<Mlt::Properties> m_sourceProperties;
    QMap<QString, QString> m_originalProperties;
    QMap<QString, QString> m_clipProperties;
    QList<int> m_videoStreams;
    QTreeWidget *m_propertiesTree;
    QWidget *m_propertiesPage;
    QWidget *m_metaPage;
    QWidget *m_analysisPage;
    QComboBox *m_audioStream;
    AnalysisTree *m_analysisTree;
    QTextEdit *m_textEdit;
    QListWidget *m_audioStreamsView;
    QGroupBox *m_audioEffectGroup;
    QCheckBox *m_swapChannels;
    QCheckBox *m_normalize;
    QButtonGroup *m_copyChannelGroup;
    QCheckBox *m_copyChannel1;
    QCheckBox *m_copyChannel2;
    QSpinBox *m_gain;
    KMessageWidget m_warningMessage;
    /** @brief The selected audio stream. */
    int m_activeAudioStreams;
    void fillProperties();
    /** @brief Add/remove icon beside audio stream to indicate effects. */
    void updateStreamIcon(int row, int streamIndex);
    /** @brief Create a rotated icon to display in side tabs. */
    QIcon rotatedIcon(const QSize iconSize, const QString &iconName);

Q_SIGNALS:
    void updateClipProperties(const QString &, const QMap<QString, QString> &, const QMap<QString, QString> &);
    void colorModified(const QColor &);
    void modified(int);
    void updateTimeCodeFormat();
    /** @brief Seek clip monitor to a frame. */
    void seekToFrame(int);
    void editAnalysis(const QString &id, const QString &name, const QString &value);
    void editClip();
    void requestProxy(bool doProxy);
    void proxyModified(const QString &);
    void deleteProxy();
    void enableProxy(bool);
};
