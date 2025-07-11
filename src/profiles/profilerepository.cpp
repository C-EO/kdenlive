/*
    SPDX-FileCopyrightText: 2017 Nicolas Carion
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "profilerepository.hpp"
#include "kdenlive_debug.h"
#include "kdenlivesettings.h"
#include "profilemodel.hpp"
#include <KLocalizedString>
#include <KMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>
#include <mlt++/MltProfile.h>

std::unique_ptr<ProfileRepository> ProfileRepository::instance;
std::once_flag ProfileRepository::m_onceFlag;
std::vector<std::pair<int, QString>> ProfileRepository::colorProfiles{{601, QStringLiteral("ITU-R BT.601")},       {709, QStringLiteral("ITU-R BT.709")},
                                                                      {240, QStringLiteral("SMPTE ST240")},        {170, QStringLiteral("SMPTE ST170")},
                                                                      {2020, QStringLiteral("ITU-R BT.2020 NCL")}, {2021, QStringLiteral("ITU-R BT.2020 CL")},
                                                                      {470, QStringLiteral("ITU-R BT.470 BG")}};

ProfileRepository::ProfileRepository()
{
    refresh();
}

std::unique_ptr<ProfileRepository> &ProfileRepository::get()
{
    std::call_once(m_onceFlag, [] { instance.reset(new ProfileRepository()); });
    return instance;
}

void ProfileRepository::refresh()
{
    QWriteLocker locker(&m_mutex);

    // Helper function to check a profile and print debug info
    auto check_profile = [&](std::unique_ptr<ProfileModel> &profile, const QString &file) {
        if (m_profiles.count(file) > 0) {
            return false;
        }
        if (!profile->is_valid()) {
            qCWarning(KDENLIVE_LOG) << "//// WARNING: invalid profile found: " << file << ". Ignoring.";
            return false;
        }
        return true;
    };

    // list MLT profiles.
    QDir mltDir(KdenliveSettings::mltpath());
    QStringList profilesFiles = mltDir.entryList(QDir::Files);

    // list Custom Profiles
    QStringList customProfilesDir = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, QStringLiteral("profiles/"), QStandardPaths::LocateDirectory);
    for (const auto &dir : std::as_const(customProfilesDir)) {
        QStringList files = QDir(dir).entryList(QDir::Files);
        for (const auto &file : std::as_const(files)) {
            profilesFiles << QDir(dir).absoluteFilePath(file);
        }
    }

    // Iterate through files
    for (const auto &file : std::as_const(profilesFiles)) {
        std::unique_ptr<ProfileModel> profile(new ProfileModel(file));
        if (check_profile(profile, file)) {
            m_profiles.insert(std::make_pair(file, std::move(profile)));
        }
    }
}

QVector<QPair<QString, QString>> ProfileRepository::getAllProfiles() const
{
    QReadLocker locker(&m_mutex);

    QVector<QPair<QString, QString>> list;
    for (const auto &profile : m_profiles) {
        list.push_back({profile.second->description(), profile.first});
    }
    std::sort(list.begin(), list.end());
    return list;
}

std::unique_ptr<ProfileModel> &ProfileRepository::getProfile(const QString &path)
{
    QReadLocker locker(&m_mutex);
    if (m_profiles.count(path) == 0) {
        qCWarning(KDENLIVE_LOG) << "Profile " << path << "not found. Returning default profile instead.";
        QString default_profile = KdenliveSettings::default_profile();
        if (default_profile.isEmpty()) {
            default_profile = QStringLiteral("dv_pal");
        }
        if (m_profiles.count(default_profile) == 0) {
            qCWarning(KDENLIVE_LOG) << "Default profile" << default_profile << "not found. Returning random profile instead.";
            return (*(m_profiles.begin())).second;
        }
        return m_profiles.at(default_profile);
    }
    return m_profiles.at(path);
}

bool ProfileRepository::profileExists(const QString &path) const
{
    QReadLocker locker(&m_mutex);
    return m_profiles.count(path) != 0;
}

// static
QString ProfileRepository::getColorspaceDescription(int colorspace)
{
    // TODO: should the descriptions be translated?
    for (const auto &cs : colorProfiles) {
        if (cs.first == colorspace) return cs.second;
    }
    return i18n("Unknown");
}

// static
int ProfileRepository::getColorspaceFromDescription(const QString &description)
{
    for (const auto &cs : colorProfiles) {
        if (cs.second == description) return cs.first;
    }
    return 0;
}

QVector<double> ProfileRepository::getAllFps() const
{
    QReadLocker locker(&m_mutex);
    QVector<double> res;
    for (const auto &ptr : m_profiles) {
        res.push_back(ptr.second->fps());
    }
    std::sort(res.begin(), res.end());
    res.erase(std::unique(res.begin(), res.end()), res.end());
    return res;
}

QString ProfileRepository::findMatchingProfile(ProfileInfo *profile) const
{
    QReadLocker locker(&m_mutex);
    for (const auto &ptr : m_profiles) {
        if (*ptr.second.get() == *profile) {
            return ptr.first;
        }
    }
    return QString();
}

const QString ProfileRepository::saveProfile(ProfileInfo *profile, QString profilePath)
{
    bool newProfile = false;
    if (profilePath.isEmpty()) {
        int i = 0;
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/profiles/"));
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }
        QString customName = QStringLiteral("customprofile");
        profilePath = dir.absoluteFilePath(customName + QString::number(i));
        while (QFile::exists(profilePath)) {
            ++i;
            profilePath = dir.absoluteFilePath(customName + QString::number(i));
        }
    }
    QFile file(profilePath);
    if (file.exists()) {
        newProfile = false;
    }
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(nullptr, i18n("Cannot open file %1", profilePath));
        return QString();
    }
    QTextStream out(&file);
    out << "description=" << profile->description() << '\n'
        << "frame_rate_num=" << profile->frame_rate_num() << '\n'
        << "frame_rate_den=" << profile->frame_rate_den() << '\n'
        << "width=" << profile->width() << '\n'
        << "height=" << profile->height() << '\n'
        << "progressive=" << static_cast<int>(profile->progressive()) << '\n'
        << "bottom_field_first=" << static_cast<int>(profile->bottom_field_first()) << '\n'
        << "sample_aspect_num=" << profile->sample_aspect_num() << '\n'
        << "sample_aspect_den=" << profile->sample_aspect_den() << '\n'
        << "display_aspect_num=" << profile->display_aspect_num() << '\n'
        << "display_aspect_den=" << profile->display_aspect_den() << '\n'
        << "colorspace=" << profile->colorspace() << '\n';
    if (file.error() != QFile::NoError) {
        KMessageBox::error(nullptr, i18n("Cannot write to file %1", profilePath));
        profilePath.clear();
    }
    file.close();
    if (!newProfile) {
        // We edited an existing profile, remove it to trigger a reload
        if (m_profiles.count(profilePath) > 0) {
            m_profiles.erase(profilePath);
        }
    }
    refresh();
    return profilePath;
}

bool ProfileRepository::deleteProfile(const QString &path)
{
    bool success = false;
    if (path.contains(QLatin1Char('/'))) {
        success = QFile::remove(path);
    }
    if (!success) {
        qCDebug(KDENLIVE_LOG) << "//// Cannot delete profile " << path << ", does not seem to be custom one";
    } else {
        if (m_profiles.count(path) > 0) {
            // remove the stored profile
            m_profiles.erase(path);
        }
        refresh();
    }
    return success;
}
