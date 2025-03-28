/*
SPDX-FileCopyrightText: 2018 Jean-Baptiste Mardelle <jb@kdenlive.org>
This file is part of Kdenlive. See www.kdenlive.org.

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "definitions.h"
#include <QPersistentModelIndex>

#include <QObject>
#include <QVariant>

#include <memory>

class Monitor;
class AssetParameterModel;

/** @class KeyframeMonitorHelper
    @brief This class helps manage effects that receive data from the monitor's qml overlay to translate
    the data and pass it to the model
    */
class KeyframeMonitorHelper : public QObject
{
    Q_OBJECT

public:
    /** @brief Construct a keyframe list bound to the given effect
       @param init_value is the value taken by the param at time 0.
       @param model is the asset this parameter belong to
       @param index is the index of this parameter in its model
     */
    explicit KeyframeMonitorHelper(Monitor *monitor, std::shared_ptr<AssetParameterModel> model, const QPersistentModelIndex &index, MonitorSceneType sceneType,
                                   QObject *parent = nullptr);
    /** @brief Send signals to the monitor to update the qml overlay.
       @param returns : true if the monitor's connection was changed to active.
    */
    virtual bool connectMonitor(bool activate);
    /** @brief Send data update to the monitor
     */
    virtual void refreshParams(int pos);
    /** @brief Wait until monitor scene is active to send data
     */
    void refreshParamsWhenReady(int pos);

    /** @brief Returns true if the monitor is playing
     */
    bool isPlaying() const;

    QList<QPersistentModelIndex> getIndexes();

protected:
    Monitor *m_monitor;
    std::shared_ptr<AssetParameterModel> m_model;
    /** @brief List of indexes managed by this class
     */
    QList<QPersistentModelIndex> m_indexes;
    bool m_active;
    MonitorSceneType m_requestedSceneType{MonitorSceneNone};

private Q_SLOTS:
    virtual void slotUpdateFromMonitorData(const QVariantList &v);

public Q_SLOTS:
    /** @brief For classes that manage several parameters, add a param index to the list
     */
    void addIndex(const QPersistentModelIndex &index);

Q_SIGNALS:
    /** @brief Send updated keyframe data to the parameter \@index
     */
    void updateKeyframeData(QPersistentModelIndex index, const QVariant &v);
};
