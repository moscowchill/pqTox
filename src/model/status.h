/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2019 by The qTox Project Contributors
 * Copyright © 2024-2025 The TokTok team.
 */

#include <QPixmap>
#include <QString>

#pragma once

namespace Status {
// Status::Status is weird, but Status is a fitting name for both the namespace and enum class..
enum class Status
{
    Online = 0,
    Away,
    Busy,
    Offline,
    Blocked,
};

/**
 * @brief Post-quantum identity verification status.
 *
 * Indicates the level of quantum-resistant identity verification for a connection.
 * This maps to the toxcore Tox_Connection_Identity enum.
 */
enum class IdentityStatus
{
    Unknown = 0,      ///< Not connected or status unknown
    Classical = 1,    ///< Connected with X25519 only (vulnerable to quantum attacks)
    PqUnverified = 2, ///< PQ hybrid session, but ML-KEM commitment not verified
    PqVerified = 3,   ///< Full PQ with verified ML-KEM commitment (quantum-resistant)
};

QString getIconPath(Status status, bool event = false);
QString getTitle(Status status);
QString getAssetSuffix(Status status);
bool isOnline(Status status);

// Post-quantum identity status helpers
QString getIdentityStatusTitle(IdentityStatus status);
QString getIdentityStatusIconPath(IdentityStatus status);
QString getIdentityStatusDescription(IdentityStatus status);
bool isPqProtected(IdentityStatus status);
} // namespace Status
