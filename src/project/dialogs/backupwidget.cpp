/*
    SPDX-FileCopyrightText: 2011 Jean-Baptiste Mardelle <jb@kdenlive.org>

SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "backupwidget.h"
#include "core.h"
#include "kdenlivesettings.h"

#include <QDir>
#include <QPushButton>

BackupWidget::BackupWidget(const QUrl &projectUrl, QUrl projectFolder, const QString &projectId, QWidget *parent)
    : QDialog(parent)
    , m_projectFolder(std::move(projectFolder))
{
    setupUi(this);
    setWindowTitle(i18nc("@title:window", "Restore Backup File"));

    if (!projectUrl.isValid()) {
        // No url, means we opened the backup dialog from an empty project
        info_label->setText(i18n("Showing all backup files in folder"));
        m_projectWildcard = QLatin1Char('*');
    } else {
        info_label->setText(i18n("Showing backup files for %1", projectUrl.fileName()));
        m_projectWildcard = projectUrl.fileName().section(QLatin1Char('.'), 0, -2);
        if (!projectId.isEmpty()) {
            m_projectWildcard.append(QLatin1Char('-') + projectId);
        } else {
            // No project id, it was lost, add wildcard
            m_projectWildcard.append(QLatin1Char('*'));
        }
    }
    m_projectWildcard.append(QStringLiteral("-??"));
    m_projectWildcard.append(QStringLiteral("??"));
    m_projectWildcard.append(QStringLiteral("-??"));
    m_projectWildcard.append(QStringLiteral("-??"));
    m_projectWildcard.append(QStringLiteral("-??"));
    m_projectWildcard.append(QStringLiteral("-??.kdenlive"));
    QAction *openContainingFolder = new QAction(QIcon::fromTheme(QStringLiteral("edit-find")), i18n("Open Containing Folder"), this);
    connect(openContainingFolder, &QAction::triggered, [&]() {
        if (backup_list->currentItem()) {
            const QString fileName = backup_list->currentItem()->data(Qt::UserRole).toString();
            pCore->highlightFileInExplorer({QUrl::fromLocalFile(fileName)});
        }
    });
    backup_list->addAction(openContainingFolder);
    backup_list->setContextMenuPolicy(Qt::ActionsContextMenu);
    slotParseBackupFiles();
    search_list->setListWidget(backup_list);
    connect(backup_list, &QListWidget::currentRowChanged, this, &BackupWidget::slotDisplayBackupPreview);
    backup_list->setCurrentRow(0);
    backup_list->setMinimumHeight(QFontMetrics(font()).lineSpacing() * 12);
}

BackupWidget::~BackupWidget() = default;

void BackupWidget::slotParseBackupFiles()
{
    QLocale locale;
    QStringList filter;
    filter << m_projectWildcard;
    backup_list->clear();

    // Parse new XDG backup folder $HOME/.local/share/kdenlive/.backup
    QDir backupFolder(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/.backup"));
    backupFolder.setNameFilters(filter);
    QFileInfoList resultList = backupFolder.entryInfoList(QDir::Files, QDir::Time);
    for (int i = 0; i < resultList.count(); ++i) {
        const QString fileName = resultList.at(i).baseName();
        QString label = getDateFromName(locale, fileName);
        if (label.isEmpty()) {
            // Use file timestamp
            label = locale.toString(resultList.at(i).lastModified(), QLocale::ShortFormat);
        }
        if (m_projectWildcard.startsWith(QLatin1Char('*'))) {
            // Displaying all backup files, so add project name in the entries
            label.prepend(resultList.at(i).fileName().section(QLatin1Char('-'), 0, -7) + QStringLiteral(".kdenlive - "));
        }
        auto *item = new QListWidgetItem(label, backup_list);
        item->setData(Qt::UserRole, resultList.at(i).absoluteFilePath());
        item->setToolTip(resultList.at(i).absoluteFilePath());
    }

    // Parse old $HOME/kdenlive/.backup folder
    QDir dir(m_projectFolder.toLocalFile() + QStringLiteral("/.backup"));
    if (dir.exists()) {
        dir.setNameFilters(filter);
        QFileInfoList resultList2 = dir.entryInfoList(QDir::Files, QDir::Time);
        for (int i = 0; i < resultList2.count(); ++i) {
            // Get modified time from filename
            const QString fileName = resultList2.at(i).baseName();
            QString label = getDateFromName(locale, fileName);
            if (label.isEmpty()) {
                // Use file timestamp
                label = locale.toString(resultList2.at(i).lastModified(), QLocale::ShortFormat);
            }
            if (m_projectWildcard.startsWith(QLatin1Char('*'))) {
                // Displaying all backup files, so add project name in the entries
                label.prepend(resultList2.at(i).fileName().section(QLatin1Char('-'), 0, -7) + QStringLiteral(".kdenlive - "));
            }
            auto *item = new QListWidgetItem(label, backup_list);
            item->setData(Qt::UserRole, resultList2.at(i).absoluteFilePath());
            item->setToolTip(resultList2.at(i).absoluteFilePath());
        }
    }

    buttonBox->button(QDialogButtonBox::Open)->setEnabled(backup_list->count() > 0);
}

const QString BackupWidget::getDateFromName(const QLocale locale, const QString &fileName)
{
    QStringList dateData = fileName.section(QLatin1Char('-'), -5).split(QLatin1Char('-'));
    QString label;
    if (dateData.size() == 5) {
        QDate date(dateData.at(0).toInt(), dateData.at(1).toInt(), dateData.at(2).toInt());
        QTime time(dateData.at(3).toInt(), dateData.at(4).toInt());
        QDateTime dt(date, time);
        if (dt.isValid()) {
            label = locale.toString(dt, QLocale::ShortFormat);
        }
    }
    return label;
}

void BackupWidget::slotDisplayBackupPreview()
{
    if (!backup_list->currentItem()) {
        backup_preview->setPixmap(QPixmap());
        return;
    }
    const QString path = backup_list->currentItem()->data(Qt::UserRole).toString();
    QPixmap pix(path + QStringLiteral(".jpg"));
    backup_preview->setPixmap(pix);
}

QString BackupWidget::selectedFile() const
{
    if (!backup_list->currentItem()) {
        return QString();
    }
    return backup_list->currentItem()->data(Qt::UserRole).toString();
}
