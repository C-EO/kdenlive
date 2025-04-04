/*
    SPDX-FileCopyrightText: 2015 Meltytech LLC
    SPDX-FileCopyrightText: 2015 Brian Matherly <code@brianmatherly.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "scopewidget.h"
#include "core.h"
#include "kdenlive_debug.h"
#include <QtConcurrent/QtConcurrentRun>

ScopeWidget::ScopeWidget(QWidget *parent)
    : QWidget(parent)
    , m_queue(3, DataQueue<SharedFrame>::OverflowModeDiscardOldest)
    , m_future()
    , m_mutex()
    , m_size(0, 0)
{
}

ScopeWidget::~ScopeWidget() = default;

void ScopeWidget::onNewFrame(const SharedFrame &frame)
{
    m_queue.push(frame);
    requestRefresh();
}

void ScopeWidget::requestRefresh()
{
    if (m_future.isFinished()) {
        m_future = QtConcurrent::run(&ScopeWidget::refreshInThread, this);
    } else {
        m_refreshPending = true;
    }
}

void ScopeWidget::refreshInThread()
{
    if (m_size.isEmpty() && !pCore->audioMixerVisible) {
        return;
    }

    m_mutex.lock();
    QSize size = m_size;
    bool full = m_forceRefresh;
    m_forceRefresh = false;
    m_mutex.unlock();

    m_refreshPending = false;
    refreshScope(size, full);
    // Tell the GUI thread that the refresh is complete.
    QMetaObject::invokeMethod(this, "onRefreshThreadComplete", Qt::QueuedConnection);
}

void ScopeWidget::onRefreshThreadComplete()
{
    update();
    if (m_refreshPending) {
        requestRefresh();
    }
}

void ScopeWidget::resizeEvent(QResizeEvent *)
{
    m_mutex.lock();
    m_size = size();
    m_mutex.unlock();
    requestRefresh();
}

void ScopeWidget::changeEvent(QEvent *)
{
    m_mutex.lock();
    m_forceRefresh = true;
    m_mutex.unlock();
    requestRefresh();
}
