/*
    SPDX-FileCopyrightText: 2018 Jean-Baptiste Mardelle
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "urlparamwidget.hpp"
#include "assets/model/assetparametermodel.hpp"

#include <KUrlRequester>

UrlParamWidget::UrlParamWidget(std::shared_ptr<AssetParameterModel> model, QModelIndex index, QWidget *parent)
    : AbstractParamWidget(std::move(model), index, parent)
{
    setupUi(this);

    labelComment->setHidden(true);
    QString filter = m_model->data(m_index, AssetParameterModel::FilterRole).toString();
    if (!filter.isEmpty()) {
        urlwidget->setNameFilter(filter);
    }
    QString mode = m_model->data(m_index, AssetParameterModel::ModeRole).toString();
    if (mode == QLatin1String("save")) {
        urlwidget->setAcceptMode(QFileDialog::AcceptSave);
        urlwidget->setMode(KFile::File);
    }
    setMinimumHeight(urlwidget->sizeHint().height());
    // set check state
    slotRefresh();

    // Q_EMIT the signal of the base class when appropriate
    connect(this->urlwidget, &KUrlRequester::textChanged, this, [this]() {
        QFileInfo info(urlwidget->url().toLocalFile());
        if (info.exists() && info.isFile()) {
            Q_EMIT valueChanged(m_index, this->urlwidget->url().toLocalFile(), true);
        }
    });
}

void UrlParamWidget::slotShowComment(bool show)
{
    if (!labelComment->text().isEmpty()) {
        labelComment->setVisible(show);
    }
}

void UrlParamWidget::slotRefresh()
{
    const QSignalBlocker bk(urlwidget);
    urlwidget->setUrl(QUrl::fromLocalFile(m_model->data(m_index, AssetParameterModel::ValueRole).toString()));
}
