/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2019 by The qTox Project Contributors
 * Copyright © 2024-2025 The TokTok team.
 */

#include "status.h"

#include <QDebug>
#include <QObject>
#include <QPixmap>
#include <QString>

#include <cassert>

namespace Status {
QString getTitle(Status status)
{
    switch (status) {
    case Status::Online:
        return QObject::tr("online", "contact status");
    case Status::Away:
        return QObject::tr("away", "contact status");
    case Status::Busy:
        return QObject::tr("busy", "contact status");
    case Status::Offline:
        return QObject::tr("offline", "contact status");
    case Status::Blocked:
        return QObject::tr("blocked", "contact status");
    }

    assert(false);
    return QStringLiteral("");
}

QString getAssetSuffix(Status status)
{
    switch (status) {
    case Status::Online:
        return "online";
    case Status::Away:
        return "away";
    case Status::Busy:
        return "busy";
    case Status::Offline:
        return "offline";
    case Status::Blocked:
        return "blocked";
    }
    assert(false);
    return QStringLiteral("");
}

QString getIconPath(Status status, bool event)
{
    const QString eventSuffix = event ? QStringLiteral("_notification") : QString();
    const QString statusSuffix = getAssetSuffix(status);
    if (status == Status::Blocked) {
        return ":/img/status/" + statusSuffix + ".svg";
    }
    return ":/img/status/" + statusSuffix + eventSuffix + ".svg";
}

bool isOnline(Status status)
{
    return status != Status::Offline && status != Status::Blocked;
}

QString getIdentityStatusTitle(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::Unknown:
        return QObject::tr("Unknown", "identity status");
    case IdentityStatus::Classical:
        return QObject::tr("Classical", "identity status");
    case IdentityStatus::PqUnverified:
        return QObject::tr("PQ Unverified", "identity status");
    case IdentityStatus::PqVerified:
        return QObject::tr("PQ Verified", "identity status");
    }
    assert(false);
    return QStringLiteral("");
}

QString getIdentityStatusIconPath(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::Unknown:
        return ":/img/security/shield_gray.svg";
    case IdentityStatus::Classical:
        return ":/img/security/shield_yellow.svg";
    case IdentityStatus::PqUnverified:
        return ":/img/security/shield_blue.svg";
    case IdentityStatus::PqVerified:
        return ":/img/security/shield_green.svg";
    }
    assert(false);
    return QStringLiteral("");
}

QString getIdentityStatusDescription(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::Unknown:
        return QObject::tr("Not connected", "identity status description");
    case IdentityStatus::Classical:
        return QObject::tr("Classical encryption (X25519) - not quantum-resistant",
                           "identity status description");
    case IdentityStatus::PqUnverified:
        return QObject::tr("Post-quantum encryption active, but identity not verified",
                           "identity status description");
    case IdentityStatus::PqVerified:
        return QObject::tr(
            "Post-quantum encryption with verified identity - fully quantum-resistant",
            "identity status description");
    }
    assert(false);
    return QStringLiteral("");
}

bool isPqProtected(IdentityStatus status)
{
    return status == IdentityStatus::PqUnverified || status == IdentityStatus::PqVerified;
}
} // namespace Status
