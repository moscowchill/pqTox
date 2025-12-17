# pqTox Implementation Guide

Step-by-step guide to implementing PQ support in qTox.

## Prerequisites

1. **PQ-enabled toxcore built and installed**
   ```bash
   cd /home/waterfall/c-toxcore-pq
   mkdir _build && cd _build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   make -j$(nproc) && sudo make install
   ```

2. **Qt 5.15+ development environment**
   ```bash
   sudo apt install qtbase5-dev qttools5-dev qtmultimedia5-dev \
       qtsvg5-dev libqt5opengl5-dev
   ```

3. **qTox cloned to pqTox**
   ```bash
   git clone https://github.com/TokTok/qTox.git /home/waterfall/pqTox
   ```

---

## Phase 1: Build System (30 min)

### Step 1.1: Add PQ Detection to CMake

**File:** `/home/waterfall/pqTox/cmake/Dependencies.cmake`

After the toxcore detection block (~line 170), add:

```cmake
# ============================================
# PQ-enabled toxcore detection
# ============================================
if(TOXCORE_FOUND)
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES ${TOXCORE_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${TOXCORE_LIBRARIES})

    # Check for PQ API functions
    check_symbol_exists(tox_friend_get_identity_status "tox/tox.h" HAVE_TOX_IDENTITY_STATUS)
    check_symbol_exists(tox_friend_add_pq "tox/tox.h" HAVE_TOX_FRIEND_ADD_PQ)

    if(HAVE_TOX_IDENTITY_STATUS AND HAVE_TOX_FRIEND_ADD_PQ)
        message(STATUS "Found PQ-enabled toxcore")
        set(QTOX_PQ_SUPPORT ON CACHE BOOL "PQ toxcore detected")
    else()
        message(STATUS "Classical toxcore (no PQ support)")
        set(QTOX_PQ_SUPPORT OFF CACHE BOOL "No PQ toxcore")
    endif()
endif()
```

### Step 1.2: Add Compile Definition

**File:** `/home/waterfall/pqTox/CMakeLists.txt`

Near other compile definitions, add:

```cmake
if(QTOX_PQ_SUPPORT)
    add_compile_definitions(QTOX_PQ_SUPPORT)
    message(STATUS "Compiling with PQ support enabled")
endif()
```

### Step 1.3: Verify Detection

```bash
cd /home/waterfall/pqTox
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
# Should see: "Found PQ-enabled toxcore"
```

---

## Phase 2: ToxId Class (1 hour)

### Step 2.1: Update Header

**File:** `/home/waterfall/pqTox/src/core/toxid.h`

```cpp
// Replace size constants with:
public:
    static constexpr int classicalSize = 38;
    static constexpr int pqSize = 46;
    static constexpr int classicalHexChars = 76;
    static constexpr int pqHexChars = 92;

    // Backwards compatibility
    static constexpr int size = classicalSize;
    static constexpr int numHexChars = classicalHexChars;

    // New methods
    bool isPqAddress() const;
    QByteArray getMlkemCommitment() const;
    static bool isClassicalToxId(const QString& id);
    static bool isPqToxId(const QString& id);

private:
    bool m_isPqAddress = false;
```

### Step 2.2: Implement Methods

**File:** `/home/waterfall/pqTox/src/core/toxid.cpp`

Add implementations:

```cpp
bool ToxId::isPqAddress() const
{
    return m_isPqAddress;
}

QByteArray ToxId::getMlkemCommitment() const
{
    if (!m_isPqAddress || toxId.size() < pqSize) {
        return QByteArray();
    }
    return toxId.mid(ToxPk::size, 8);
}

bool ToxId::isClassicalToxId(const QString& id)
{
    if (id.length() != classicalHexChars) {
        return false;
    }
    static const QRegularExpression hexRegExp(
        QString("^[A-Fa-f0-9]{%1}$").arg(classicalHexChars));
    if (!hexRegExp.match(id).hasMatch()) {
        return false;
    }
    QByteArray data = QByteArray::fromHex(id.toLatin1());
    return ToxId(data).isValid();
}

bool ToxId::isPqToxId(const QString& id)
{
    if (id.length() != pqHexChars) {
        return false;
    }
    static const QRegularExpression hexRegExp(
        QString("^[A-Fa-f0-9]{%1}$").arg(pqHexChars));
    if (!hexRegExp.match(id).hasMatch()) {
        return false;
    }
    QByteArray data = QByteArray::fromHex(id.toLatin1());
    return ToxId(data).isValid();
}

// Update isToxId to accept both
bool ToxId::isToxId(const QString& id)
{
    return isClassicalToxId(id) || isPqToxId(id);
}
```

### Step 2.3: Update Constructor

In the constructor that takes QByteArray:

```cpp
ToxId::ToxId(const QByteArray& rawId)
{
    if (rawId.size() == pqSize) {
        constructToxId(rawId);
        m_isPqAddress = true;
    } else if (rawId.size() == classicalSize) {
        constructToxId(rawId);
        m_isPqAddress = false;
    } else {
        toxId.clear();
    }
}
```

---

## Phase 3: Core Layer (1.5 hours)

### Step 3.1: Fix Assertions

**File:** `/home/waterfall/pqTox/src/core/core.cpp`

Find the size assertions (~line 60) and update:

```cpp
void Core::validateToxcoreSizes()
{
#ifdef QTOX_PQ_SUPPORT
    size_t addrSize = tox_address_size();
    if (addrSize != ToxId::classicalSize && addrSize != ToxId::pqSize) {
        qFatal("Unexpected tox_address_size: %zu", addrSize);
    }
#else
    assert(ToxId::size == tox_address_size());
#endif
    assert(ToxPk::size == tox_public_key_size());
    assert(ConferenceId::size == tox_conference_id_size());
}
```

### Step 3.2: Add Identity Callback

**File:** `/home/waterfall/pqTox/src/core/core.h`

Add to signals section:

```cpp
signals:
    // ... existing signals ...

#ifdef QTOX_PQ_SUPPORT
    void friendIdentityStatusChanged(uint32_t friendId, int status);
#endif
```

Add to private section:

```cpp
private:
#ifdef QTOX_PQ_SUPPORT
    static void onFriendIdentityStatusChanged(Tox* tox, uint32_t friendId,
                                               int status, void* vCore);
#endif
```

### Step 3.3: Implement Callback

**File:** `/home/waterfall/pqTox/src/core/core.cpp`

Add implementation:

```cpp
#ifdef QTOX_PQ_SUPPORT
void Core::onFriendIdentityStatusChanged(Tox*, uint32_t friendId,
                                          int status, void* vCore)
{
    Core* core = static_cast<Core*>(vCore);
    QMetaObject::invokeMethod(core, [=]() {
        emit core->friendIdentityStatusChanged(friendId, status);
    });
}
#endif
```

Register the callback in the constructor (after other callbacks):

```cpp
#ifdef QTOX_PQ_SUPPORT
    tox_callback_friend_identity_status(tox.get(), onFriendIdentityStatusChanged);
#endif
```

---

## Phase 4: Model Layer (45 min)

### Step 4.1: Add Identity Status Enum

**File:** `/home/waterfall/pqTox/src/model/status.h`

Add after the existing Status enum:

```cpp
#ifdef QTOX_PQ_SUPPORT
enum class IdentityStatus
{
    Unknown = 0,
    Classical = 1,
    PqUnverified = 2,
    PqVerified = 3
};

QString getIdentityStatusString(IdentityStatus status);
QString getIdentityStatusIcon(IdentityStatus status);
QColor getIdentityStatusColor(IdentityStatus status);
#endif
```

### Step 4.2: Implement Helpers

**File:** `/home/waterfall/pqTox/src/model/status.cpp`

```cpp
#ifdef QTOX_PQ_SUPPORT
QString Status::getIdentityStatusString(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::Classical:
        return QCoreApplication::translate("Status", "Classical");
    case IdentityStatus::PqUnverified:
        return QCoreApplication::translate("Status", "Quantum-Safe");
    case IdentityStatus::PqVerified:
        return QCoreApplication::translate("Status", "Quantum-Verified");
    default:
        return QCoreApplication::translate("Status", "Unknown");
    }
}

QString Status::getIdentityStatusIcon(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::PqVerified:
        return QStringLiteral(":/img/status/shield_green.svg");
    case IdentityStatus::PqUnverified:
        return QStringLiteral(":/img/status/shield_blue.svg");
    case IdentityStatus::Classical:
        return QStringLiteral(":/img/status/shield_yellow.svg");
    default:
        return QStringLiteral(":/img/status/shield_gray.svg");
    }
}

QColor Status::getIdentityStatusColor(IdentityStatus status)
{
    switch (status) {
    case IdentityStatus::PqVerified:
        return QColor(76, 175, 80);    // Green
    case IdentityStatus::PqUnverified:
        return QColor(33, 150, 243);   // Blue
    case IdentityStatus::Classical:
        return QColor(255, 193, 7);    // Yellow/Amber
    default:
        return QColor(158, 158, 158);  // Gray
    }
}
#endif
```

### Step 4.3: Update Friend Model

**File:** `/home/waterfall/pqTox/src/model/friend.h`

Add field and accessor:

```cpp
public:
#ifdef QTOX_PQ_SUPPORT
    Status::IdentityStatus getIdentityStatus() const { return identityStatus_; }
    void setIdentityStatus(Status::IdentityStatus s);

signals:
    void identityStatusChanged(const ToxPk&, Status::IdentityStatus);

private:
    Status::IdentityStatus identityStatus_ = Status::IdentityStatus::Unknown;
#endif
```

---

## Phase 5: UI Components (2 hours)

### Step 5.1: Create Shield Icons

Create 4 SVG files in `/home/waterfall/pqTox/res/img/status/`:

**shield_green.svg** (PQ Verified):
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24">
  <path fill="#4CAF50" d="M12 1L3 5v6c0 5.55 3.84 10.74 9 12 5.16-1.26 9-6.45 9-12V5l-9-4zm-2 16l-4-4 1.41-1.41L10 14.17l6.59-6.59L18 9l-8 8z"/>
</svg>
```

**shield_blue.svg** (PQ Unverified):
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24">
  <path fill="#2196F3" d="M12 1L3 5v6c0 5.55 3.84 10.74 9 12 5.16-1.26 9-6.45 9-12V5l-9-4zm0 10.99h7c-.53 4.12-3.28 7.79-7 8.94V12H5V6.3l7-3.11v8.8z"/>
</svg>
```

**shield_yellow.svg** (Classical):
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24">
  <path fill="#FFC107" d="M12 1L3 5v6c0 5.55 3.84 10.74 9 12 5.16-1.26 9-6.45 9-12V5l-9-4zm0 10.99h7c-.53 4.12-3.28 7.79-7 8.94V12H5V6.3l7-3.11v8.8z"/>
</svg>
```

**shield_gray.svg** (Unknown):
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24">
  <path fill="#9E9E9E" d="M12 1L3 5v6c0 5.55 3.84 10.74 9 12 5.16-1.26 9-6.45 9-12V5l-9-4zm0 10.99h7c-.53 4.12-3.28 7.79-7 8.94V12H5V6.3l7-3.11v8.8z"/>
</svg>
```

### Step 5.2: Add to Resources

**File:** `/home/waterfall/pqTox/res.qrc`

Add inside the appropriate qresource block:

```xml
<file>img/status/shield_green.svg</file>
<file>img/status/shield_blue.svg</file>
<file>img/status/shield_yellow.svg</file>
<file>img/status/shield_gray.svg</file>
```

### Step 5.3: Update Add Friend Form

**File:** `/home/waterfall/pqTox/src/widget/form/addfriendform.cpp`

Update validation and placeholder:

```cpp
void AddFriendForm::retranslateUi()
{
#ifdef QTOX_PQ_SUPPORT
    toxIdLabel.setText(tr("Tox ID (76 or 92 hex characters):"));
#else
    toxIdLabel.setText(tr("Tox ID (76 hexadecimal characters):"));
#endif
}

bool AddFriendForm::checkIsValidId(const QString& id)
{
    return ToxId::isToxId(id);
}
```

---

## Phase 6: Testing (1 hour)

### Step 6.1: Build pqTox

```bash
cd /home/waterfall/pqTox/build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local \
         -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Step 6.2: Run Unit Tests

```bash
cd /home/waterfall/pqTox/build
ctest --output-on-failure
```

### Step 6.3: Manual Testing Checklist

- [ ] Add friend with 76-char classical address
- [ ] Add friend with 92-char PQ address
- [ ] Verify shield icon displays correctly
- [ ] Check identity status in About Friend dialog
- [ ] Verify tooltip shows correct status text
- [ ] Test with classical toxcore peer (should show yellow shield)
- [ ] Test with PQ toxcore peer (should show green shield)

---

## Phase 7: Integration Test

### Create Test Script

**File:** `/home/waterfall/pqTox/test_pq_integration.sh`

```bash
#!/bin/bash
set -e

echo "=== pqTox PQ Integration Test ==="

# Build test
cd /home/waterfall/pqTox/build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
make -j$(nproc)

# Check PQ support compiled in
if ./qtox --version 2>&1 | grep -q "PQ"; then
    echo "✓ PQ support detected in build"
else
    echo "✗ PQ support NOT detected"
    exit 1
fi

# Run existing tests
ctest --output-on-failure -R toxid

echo "=== All tests passed ==="
```

---

## Troubleshooting

### "tox_friend_get_identity_status not found"

Your toxcore doesn't have PQ support. Rebuild c-toxcore-pq:
```bash
cd /home/waterfall/c-toxcore-pq/_build
cmake .. && make && sudo make install
sudo ldconfig
```

### "ToxId size assertion failed"

The ToxId class hasn't been updated properly. Check that both `classicalSize` and `pqSize` constants are defined.

### "Shield icons not showing"

1. Check icons are in `res/img/status/`
2. Verify they're added to `res.qrc`
3. Run `qmake` or reconfigure cmake to regenerate resources

### "PQ detection not working"

Check cmake output for "Found PQ-enabled toxcore" message. If missing:
```bash
# Verify toxcore has PQ symbols
nm -D /usr/local/lib/libtoxcore.so | grep identity_status
```
