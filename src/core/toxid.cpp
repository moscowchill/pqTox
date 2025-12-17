/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2015-2019 by The qTox Project Contributors
 * Copyright © 2024-2025 The TokTok team.
 */


#include "toxid.h"

#include "chatid.h"
#include "toxpk.h"

#include <QRegularExpression>

#include <cassert>
#include <cstdint>

// Classical address regex (76 hex chars = 38 bytes)
const QRegularExpression
    ToxId::ToxIdRegEx(QString("(^|\\s)[A-Fa-f0-9]{%1}($|\\s)").arg(ToxId::numHexChars));

// Post-quantum address regex (92 hex chars = 46 bytes)
const QRegularExpression
    ToxId::ToxIdRegExPq(QString("(^|\\s)[A-Fa-f0-9]{%1}($|\\s)").arg(ToxId::numHexCharsPq));

/**
 * @class ToxId
 * @brief This class represents a Tox ID.
 *
 * Classical ID (38 bytes, 76 hex chars):
 * An ID is composed of 32 bytes long public key, 4 bytes long NoSpam
 * and 2 bytes long checksum.
 *
 * e.g.
 * @code
 * | C7719C6808C14B77348004956D1D98046CE09A34370E7608150EAD74C3815D30 | C8BA3AB9 | BEB9
 * |                                                                 /           |
 * |                                                                /    NoSpam  | Checksum
 * |           Public Key (PK), 32 bytes, 64 characters            /    4 bytes  |  2 bytes
 * |                                                              |  8 characters|  4 characters
 * @endcode
 *
 * Post-Quantum ID (46 bytes, 92 hex chars):
 * A PQ ID adds an 8-byte ML-KEM commitment for quantum-resistant identity verification.
 *
 * e.g.
 * @code
 * | Public Key (32 bytes) | ML-KEM Commitment (8 bytes) | NoSpam (4 bytes) | Checksum (2 bytes)
 * @endcode
 *
 * The ML-KEM commitment is SHA256(ML-KEM public key)[0:8], enabling verification
 * that a peer's ML-KEM key matches their claimed identity even against quantum attackers.
 */

/**
 * @brief The default constructor. Creates an empty Tox ID.
 */
ToxId::ToxId()
    : toxId()
{
}

/**
 * @brief The copy constructor.
 * @param other ToxId to copy.
 */
ToxId::ToxId(const ToxId& other) = default;

/**
 * @brief The move constructor.
 * @param other ToxId to move from.
 */
ToxId::ToxId(ToxId&& other) = default;

/**
 * @brief Create a Tox ID from a QString.
 *
 * Supports both classical (76 hex chars) and PQ (92 hex chars) addresses.
 * If the given rawId is not a valid Tox ID, but can be a Public Key then:
 * publicKey == rawId and noSpam == 0 == checkSum.
 * If the given rawId isn't a valid Public Key or Tox ID a ToxId with all zero bytes is created.
 *
 * @param id Tox ID string to convert to ToxId object
 */
ToxId::ToxId(const QString& id)
{
    if (isToxId(id)) {
        toxId = QByteArray::fromHex(id.toLatin1());
    } else {
        assert(!"ToxId constructed with invalid length string");
        toxId = QByteArray(); // invalid id string
    }
}

/**
 * @brief Create a Tox ID from a QByteArray.
 *
 * If the given rawId is not a valid Tox ID, but can be a Public Key then:
 * publicKey == rawId and noSpam == 0 == checkSum.
 * If the given rawId isn't a valid Public Key or Tox ID a ToxId with all zero bytes is created.
 *
 * @param rawId Tox ID bytes to convert to ToxId object
 */
ToxId::ToxId(const QByteArray& rawId)
{
    constructToxId(rawId);
}

/**
 * @brief Create a Tox ID from uint8_t bytes and length, convenience function for toxcore interface.
 *
 * If the given rawId is not a valid Tox ID, but can be a Public Key then:
 * publicKey == rawId and noSpam == 0 == checkSum.
 * If the given rawId isn't a valid Public Key or Tox ID a ToxId with all zero bytes is created.
 *
 * @param rawId Pointer to bytes to convert to ToxId object
 * @param len Number of bytes to read. Must be ToxPk::size for a Public Key or
 *            ToxId::size for a Tox ID.
 */
ToxId::ToxId(const uint8_t* rawId, int len)
{
    const QByteArray tmpId(reinterpret_cast<const char*>(rawId), len);
    constructToxId(tmpId);
}


void ToxId::constructToxId(const QByteArray& rawId)
{
    const QString hexId = QString::fromUtf8(rawId.toHex()).toUpper();
    // Accept both classical (38 bytes) and PQ (46 bytes) addresses
    if ((rawId.length() == ToxId::size || rawId.length() == ToxId::sizePq) && isToxId(hexId)) {
        toxId = QByteArray(rawId); // construct from full tox id
    } else {
        assert(!"ToxId constructed with invalid input");
        toxId = QByteArray(); // invalid id
    }
}

/**
 * @brief Compares the equality of the Public Key.
 * @param other Tox ID to compare.
 * @return True if both Tox IDs have the same public keys, false otherwise.
 */
bool ToxId::operator==(const ToxId& other) const
{
    return getPublicKey() == other.getPublicKey();
}

/**
 * @brief Compares the inequality of the Public Key.
 * @param other Tox ID to compare.
 * @return True if both Tox IDs have different public keys, false otherwise.
 */
bool ToxId::operator!=(const ToxId& other) const
{
    return getPublicKey() != other.getPublicKey();
}

/**
 * @brief Returns the Tox ID converted to QString.
 * Is equal to getPublicKey() if the Tox ID was constructed from only a Public Key.
 * @return The Tox ID as QString.
 */
QString ToxId::toString() const
{
    return QString::fromUtf8(toxId.toHex()).toUpper();
}

/**
 * @brief Clears all elements of the Tox ID.
 */
void ToxId::clear()
{
    toxId.clear();
}

/**
 * @brief Gets the ToxID as bytes, convenience function for toxcore interface.
 * @return The ToxID as uint8_t* if isValid() is true, else a nullptr.
 */
const uint8_t* ToxId::getBytes() const
{
    if (isValid()) {
        return reinterpret_cast<const uint8_t*>(toxId.constData());
    }

    return nullptr;
}

/**
 * @brief Gets the Public Key part of the ToxID
 * @return Public Key of the ToxID
 */
ToxPk ToxId::getPublicKey() const
{
    const auto pkBytes = toxId.left(ToxPk::size);
    if (pkBytes.isEmpty()) {
        return ToxPk{};
    }
    return ToxPk{pkBytes};
}

/**
 * @brief Returns the NoSpam value converted to QString.
 * @return The NoSpam value as QString or "" if the ToxId was constructed from a Public Key.
 */
QString ToxId::getNoSpamString() const
{
    if (toxId.length() == ToxId::size) {
        // Classical: [PK:32][NoSpam:4][Checksum:2]
        return QString::fromUtf8(toxId.mid(ToxPk::size, ToxId::nospamSize).toHex()).toUpper();
    } else if (toxId.length() == ToxId::sizePq) {
        // PQ: [PK:32][MLKEMCommitment:8][NoSpam:4][Checksum:2]
        return QString::fromUtf8(
                   toxId.mid(ToxPk::size + ToxId::mlkemCommitmentSize, ToxId::nospamSize).toHex())
            .toUpper();
    }

    return {};
}

/**
 * @brief Returns the size of the ToxId in bytes.
 * @return 38 for classical, 46 for PQ, or 0 if invalid.
 */
int ToxId::getSize() const
{
    return toxId.length();
}

/**
 * @brief Check if this is a post-quantum address.
 * @return True if this is a 46-byte PQ address, false otherwise.
 */
bool ToxId::isPqAddress() const
{
    return toxId.length() == ToxId::sizePq;
}

/**
 * @brief Get the ML-KEM commitment from a PQ address.
 * @return The 8-byte ML-KEM commitment, or empty if not a PQ address.
 */
QByteArray ToxId::getMlkemCommitment() const
{
    if (toxId.length() == ToxId::sizePq) {
        // PQ: [PK:32][MLKEMCommitment:8][NoSpam:4][Checksum:2]
        return toxId.mid(ToxPk::size, ToxId::mlkemCommitmentSize);
    }
    return {};
}

/**
 * @brief Check if a string is a classical (38-byte) Tox ID.
 * @param id String to check.
 * @return True if exactly 76 hex characters.
 */
bool ToxId::isClassicalToxId(const QString& id)
{
    return id.length() == ToxId::numHexChars && id.contains(ToxIdRegEx);
}

/**
 * @brief Check if a string is a post-quantum (46-byte) Tox ID.
 * @param id String to check.
 * @return True if exactly 92 hex characters.
 */
bool ToxId::isPqToxId(const QString& id)
{
    return id.length() == ToxId::numHexCharsPq && id.contains(ToxIdRegExPq);
}

/**
 * @brief Check, that id is a valid Tox ID.
 * @param id Tox ID to check.
 * @return True if id is a valid Tox ID, false otherwise.
 * @note Validates the checksum.
 */
bool ToxId::isValidToxId(const QString& id)
{
    return isToxId(id) && ToxId(id).isValid();
}

/**
 * @brief Check, that id is probably a valid Tox ID.
 * @param id Tox ID to check.
 * @return True if the string can be a ToxID (classical or PQ), false otherwise.
 * @note Doesn't validate checksum.
 */
bool ToxId::isToxId(const QString& id)
{
    return isClassicalToxId(id) || isPqToxId(id);
}

/**
 * @brief Check it it's a valid Tox ID by verifying the checksum
 * @return True if it is a valid Tox ID, false otherwise.
 * @note Works for both classical (38-byte) and PQ (46-byte) addresses.
 */
bool ToxId::isValid() const
{
    if (toxId.length() != ToxId::size && toxId.length() != ToxId::sizePq) {
        return false;
    }

    // Calculate how much data to checksum (everything except the checksum itself)
    const int dataLen = toxId.length() - ToxId::checksumSize;

    QByteArray data = toxId.left(dataLen);
    const QByteArray checksum = toxId.right(ToxId::checksumSize);
    QByteArray calculated(ToxId::checksumSize, 0x00);

    for (int i = 0; i < dataLen; i++) {
        calculated[i % 2] = calculated[i % 2] ^ data[i];
    }

    return calculated == checksum;
}
