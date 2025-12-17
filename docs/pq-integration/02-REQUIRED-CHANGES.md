# Required Code Changes for pqTox

This document details all code modifications needed to add PQ support to qTox.

## 1. Build System Changes

### `cmake/Dependencies.cmake`

**Location:** Lines 156-170

**Add PQ detection after toxcore is found:**

```cmake
# After existing toxcore detection (around line 170)
if(TOXCORE_FOUND)
    # Check for PQ-enabled toxcore
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES ${TOXCORE_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${TOXCORE_LIBRARIES})

    check_symbol_exists(tox_friend_get_identity_status "tox/tox.h" HAVE_TOX_PQ)
    check_symbol_exists(tox_friend_add_pq "tox/tox.h" HAVE_TOX_PQ_ADDRESS)

    if(HAVE_TOX_PQ AND HAVE_TOX_PQ_ADDRESS)
        message(STATUS "✓ Found PQ-enabled toxcore with identity verification")
        set(QTOX_PQ_SUPPORT ON)
        add_definitions(-DQTOX_PQ_SUPPORT)
    else()
        message(STATUS "Classical toxcore detected (no PQ support)")
        set(QTOX_PQ_SUPPORT OFF)
    endif()
endif()
```

### `CMakeLists.txt`

**Add option near other feature flags:**

```cmake
option(ENABLE_PQ_FEATURES "Enable post-quantum UI features (requires PQ toxcore)" ON)

if(ENABLE_PQ_FEATURES AND QTOX_PQ_SUPPORT)
    message(STATUS "PQ UI features enabled")
else()
    message(STATUS "PQ UI features disabled")
endif()
```

---

## 2. Core Layer Changes

### `src/core/toxid.h`

**Current:**
```cpp
class ToxId
{
public:
    static constexpr int size = 38;
    static constexpr int numHexChars = 76;
    // ...
};
```

**Modified:**
```cpp
class ToxId
{
public:
    // Support both classical and PQ addresses
    static constexpr int classicalSize = 38;
    static constexpr int pqSize = 46;
    static constexpr int classicalHexChars = 76;
    static constexpr int pqHexChars = 92;

    // For backwards compatibility
    static constexpr int size = classicalSize;
    static constexpr int numHexChars = classicalHexChars;

    // New methods
    bool isPqAddress() const;
    QByteArray getMlkemCommitment() const;

    // Modified validation
    static bool isToxId(const QString& id);
    static bool isClassicalToxId(const QString& id);
    static bool isPqToxId(const QString& id);

private:
    QByteArray toxId;
    bool m_isPqAddress = false;
};
```

### `src/core/toxid.cpp`

**Add PQ address support:**

```cpp
// Line ~110 - Update isToxId validation
bool ToxId::isToxId(const QString& id)
{
    return isClassicalToxId(id) || isPqToxId(id);
}

bool ToxId::isClassicalToxId(const QString& id)
{
    if (id.length() != classicalHexChars) {
        return false;
    }
    // Existing hex validation and checksum check for 38 bytes
    return validateChecksum(QByteArray::fromHex(id.toLatin1()), classicalSize);
}

bool ToxId::isPqToxId(const QString& id)
{
    if (id.length() != pqHexChars) {
        return false;
    }
    // Hex validation and checksum check for 46 bytes
    return validateChecksum(QByteArray::fromHex(id.toLatin1()), pqSize);
}

bool ToxId::isPqAddress() const
{
    return m_isPqAddress;
}

QByteArray ToxId::getMlkemCommitment() const
{
    if (!m_isPqAddress) {
        return QByteArray();
    }
    // Bytes 32-39 contain ML-KEM commitment
    return toxId.mid(ToxPk::size, 8);
}

// Update constructor to detect address type
ToxId::ToxId(const QByteArray& rawId)
{
    if (rawId.size() == pqSize) {
        toxId = rawId;
        m_isPqAddress = true;
    } else if (rawId.size() == classicalSize) {
        toxId = rawId;
        m_isPqAddress = false;
    } else {
        // Invalid
        toxId.clear();
    }
}
```

### `src/core/core.h`

**Add PQ identity signal and types:**

```cpp
// Near other includes
#ifdef QTOX_PQ_SUPPORT
#include <tox/tox.h>  // For Tox_Connection_Identity enum
#endif

class Core : public QObject, public ICoreIdHandler {
    Q_OBJECT

signals:
    // Existing signals...
    void friendStatusChanged(uint32_t friendId, Status::Status status);

#ifdef QTOX_PQ_SUPPORT
    // New PQ signal
    void friendIdentityStatusChanged(uint32_t friendId, int identityStatus);
#endif

public:
#ifdef QTOX_PQ_SUPPORT
    // Add friend with PQ address (46 bytes)
    uint32_t addFriendPq(const ToxId& id, const QString& message);

    // Get friend's PQ identity status
    int getFriendIdentityStatus(uint32_t friendId) const;
#endif

private:
#ifdef QTOX_PQ_SUPPORT
    static void onFriendIdentityStatusChanged(Tox* tox, uint32_t friendId,
                                               int status, void* vCore);
#endif
};
```

### `src/core/core.cpp`

**Update assertions (line ~60):**

```cpp
// Old:
assert(ToxId::size == tox_address_size());

// New:
#ifdef QTOX_PQ_SUPPORT
// PQ toxcore may return 46 for address size
size_t toxAddrSize = tox_address_size();
assert(toxAddrSize == ToxId::classicalSize || toxAddrSize == ToxId::pqSize);
#else
assert(ToxId::size == tox_address_size());
#endif
```

**Add callback registration (in constructor):**

```cpp
#ifdef QTOX_PQ_SUPPORT
tox_callback_friend_identity_status(tox, onFriendIdentityStatusChanged);
#endif
```

**Add callback implementation:**

```cpp
#ifdef QTOX_PQ_SUPPORT
void Core::onFriendIdentityStatusChanged(Tox* tox, uint32_t friendId,
                                          int status, void* vCore)
{
    Q_UNUSED(tox);
    Core* core = static_cast<Core*>(vCore);
    emit core->friendIdentityStatusChanged(friendId, status);
}

uint32_t Core::addFriendPq(const ToxId& id, const QString& message)
{
    if (!id.isPqAddress()) {
        qWarning() << "addFriendPq called with non-PQ address";
        return addFriend(id, message);  // Fall back
    }

    QByteArray addr = id.getBytes();
    QByteArray msg = message.toUtf8();

    Tox_Err_Friend_Add error;
    uint32_t friendId = tox_friend_add_pq(
        tox.get(),
        reinterpret_cast<const uint8_t*>(addr.constData()),
        reinterpret_cast<const uint8_t*>(msg.constData()),
        msg.size(),
        &error
    );

    // Handle errors similar to existing addFriend()
    return friendId;
}

int Core::getFriendIdentityStatus(uint32_t friendId) const
{
    Tox_Err_Friend_Query error;
    return tox_friend_get_identity_status(tox.get(), friendId, &error);
}
#endif
```

---

## 3. Model Layer Changes

### `src/model/status.h`

**Add PQ identity status enum:**

```cpp
namespace Status {

// Existing status enum
enum class Status {
    Online = 0,
    Away,
    Busy,
    Offline,
    Blocked,
};

#ifdef QTOX_PQ_SUPPORT
// New PQ identity status
enum class IdentityStatus {
    Unknown = 0,      // Not connected
    Classical = 1,    // X25519 only
    PqUnverified = 2, // PQ session, no commitment
    PqVerified = 3,   // PQ session + commitment verified
};

QString getIdentityStatusString(IdentityStatus status);
QString getIdentityStatusIcon(IdentityStatus status);
#endif

} // namespace Status
```

### `src/model/status.cpp`

**Add PQ status helpers:**

```cpp
#ifdef QTOX_PQ_SUPPORT
QString Status::getIdentityStatusString(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::Classical:
        return QObject::tr("Classical encryption");
    case IdentityStatus::PqUnverified:
        return QObject::tr("Post-quantum (unverified)");
    case IdentityStatus::PqVerified:
        return QObject::tr("Post-quantum verified");
    default:
        return QObject::tr("Unknown");
    }
}

QString Status::getIdentityStatusIcon(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::Classical:
        return ":/img/status/shield_yellow.svg";
    case IdentityStatus::PqUnverified:
        return ":/img/status/shield_blue.svg";
    case IdentityStatus::PqVerified:
        return ":/img/status/shield_green.svg";
    default:
        return ":/img/status/shield_gray.svg";
    }
}
#endif
```

### `src/model/friend.h`

**Add identity status field:**

```cpp
class Friend : public Chat
{
    Q_OBJECT

public:
#ifdef QTOX_PQ_SUPPORT
    Status::IdentityStatus getIdentityStatus() const;
    void setIdentityStatus(Status::IdentityStatus status);

signals:
    void identityStatusChanged(Status::IdentityStatus status);

private:
    Status::IdentityStatus identityStatus_ = Status::IdentityStatus::Unknown;
#endif
};
```

---

## 4. UI Layer Changes

### `src/model/about/iaboutfriend.h`

**Add PQ info to interface:**

```cpp
class IAboutFriend
{
public:
    virtual QString getName() const = 0;
    virtual QString getStatusMessage() const = 0;
    virtual ToxPk getPublicKey() const = 0;
    virtual QPixmap getAvatar() const = 0;

#ifdef QTOX_PQ_SUPPORT
    virtual Status::IdentityStatus getIdentityStatus() const = 0;
    virtual bool hasPqAddress() const = 0;
    virtual QString getMlkemCommitmentHex() const = 0;
#endif
};
```

### `src/widget/about/aboutfriendform.ui`

**Add PQ status display (XML snippet to add):**

```xml
<!-- Add after publicKey field (around line 140) -->
<item row="5" column="0">
    <widget class="QLabel" name="labelIdentityStatus">
        <property name="text">
            <string>Security:</string>
        </property>
    </widget>
</item>
<item row="5" column="1">
    <layout class="QHBoxLayout">
        <item>
            <widget class="QLabel" name="identityStatusIcon">
                <property name="minimumSize">
                    <size>
                        <width>16</width>
                        <height>16</height>
                    </size>
                </property>
            </widget>
        </item>
        <item>
            <widget class="QLabel" name="identityStatusText">
                <property name="text">
                    <string>Unknown</string>
                </property>
            </widget>
        </item>
    </layout>
</item>
```

### `src/widget/about/aboutfriendform.cpp`

**Add PQ status display logic:**

```cpp
void AboutFriendForm::updateUI()
{
    // Existing UI updates...

#ifdef QTOX_PQ_SUPPORT
    auto identityStatus = model->getIdentityStatus();
    ui->identityStatusText->setText(Status::getIdentityStatusString(identityStatus));
    ui->identityStatusIcon->setPixmap(QPixmap(Status::getIdentityStatusIcon(identityStatus)));

    // Show ML-KEM commitment if PQ address
    if (model->hasPqAddress()) {
        QString commitment = model->getMlkemCommitmentHex();
        ui->identityStatusText->setToolTip(
            tr("ML-KEM commitment: %1").arg(commitment));
    }
#endif
}
```

### `src/widget/friendwidget.cpp`

**Add PQ badge to friend list item:**

```cpp
void FriendWidget::updateStatusLight()
{
    // Existing status light update...

#ifdef QTOX_PQ_SUPPORT
    // Add PQ indicator badge
    auto identityStatus = friendItem->getIdentityStatus();
    if (identityStatus != Status::IdentityStatus::Unknown) {
        pqBadge->setVisible(true);
        pqBadge->setPixmap(QPixmap(Status::getIdentityStatusIcon(identityStatus)));
        pqBadge->setToolTip(Status::getIdentityStatusString(identityStatus));
    } else {
        pqBadge->setVisible(false);
    }
#endif
}
```

### `src/widget/chatformheader.cpp`

**Add PQ indicator to chat header:**

```cpp
void ChatFormHeader::updateCallButtons()
{
    // Existing button updates...

#ifdef QTOX_PQ_SUPPORT
    // Add security indicator next to name
    if (pqIndicator) {
        auto status = contact->getIdentityStatus();
        pqIndicator->setPixmap(QPixmap(Status::getIdentityStatusIcon(status)));
        pqIndicator->setToolTip(Status::getIdentityStatusString(status));
    }
#endif
}
```

### `src/widget/form/addfriendform.cpp`

**Support both address formats:**

```cpp
bool AddFriendForm::checkIsValidId(const QString& id)
{
    // Support both 76-char and 92-char addresses
    return ToxId::isToxId(id);
}

void AddFriendForm::onSendTriggered()
{
    ToxId toxId(ui->toxId->text());

    if (!toxId.isValid()) {
        showWarning(tr("Invalid Tox ID"));
        return;
    }

#ifdef QTOX_PQ_SUPPORT
    if (toxId.isPqAddress()) {
        // Use PQ-aware friend add
        emit addFriendPq(toxId, ui->message->toPlainText());
    } else {
        emit addFriend(toxId, ui->message->toPlainText());
    }
#else
    emit addFriend(toxId, ui->message->toPlainText());
#endif
}
```

**Update placeholder text:**

```cpp
void AddFriendForm::retranslateUi()
{
#ifdef QTOX_PQ_SUPPORT
    ui->toxId->setPlaceholderText(
        tr("Tox ID (76 or 92 characters)"));
    ui->toxId->setAccessibleDescription(
        tr("Enter friend's Tox ID - 76 characters for classical, "
           "92 characters for post-quantum addresses"));
#else
    ui->toxId->setPlaceholderText(tr("Tox ID (76 characters)"));
#endif
}
```

---

## 5. Resource Files

### New Icons Needed

Create SVG shield icons in `res/img/status/`:

| File | Description |
|------|-------------|
| `shield_green.svg` | PQ Verified |
| `shield_blue.svg` | PQ Unverified |
| `shield_yellow.svg` | Classical |
| `shield_gray.svg` | Unknown |

### `res.qrc`

**Add new icons:**

```xml
<qresource prefix="/img/status">
    <!-- Existing status icons -->
    <file>online.svg</file>
    <file>offline.svg</file>

    <!-- New PQ status icons -->
    <file>shield_green.svg</file>
    <file>shield_blue.svg</file>
    <file>shield_yellow.svg</file>
    <file>shield_gray.svg</file>
</qresource>
```

---

## 6. Summary: Files to Modify

| File | Changes |
|------|---------|
| `cmake/Dependencies.cmake` | Add PQ toxcore detection |
| `CMakeLists.txt` | Add ENABLE_PQ_FEATURES option |
| `src/core/toxid.h` | Add PQ address constants, methods |
| `src/core/toxid.cpp` | Implement PQ address validation |
| `src/core/core.h` | Add PQ signals and methods |
| `src/core/core.cpp` | Add PQ callbacks, fix assertions |
| `src/model/status.h` | Add IdentityStatus enum |
| `src/model/status.cpp` | Add status string/icon helpers |
| `src/model/friend.h` | Add identity status field |
| `src/model/about/iaboutfriend.h` | Add PQ info interface |
| `src/widget/about/aboutfriendform.ui` | Add security status UI |
| `src/widget/about/aboutfriendform.cpp` | Display PQ status |
| `src/widget/friendwidget.cpp` | Add PQ badge |
| `src/widget/chatformheader.cpp` | Add PQ indicator |
| `src/widget/form/addfriendform.cpp` | Support 46-byte addresses |
| `res.qrc` | Add shield icons |

**New Files:**
- `res/img/status/shield_*.svg` (4 icons)
