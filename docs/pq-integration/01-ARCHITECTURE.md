# pqTox Architecture: qTox + toxcore Integration

## Overview

qTox is a Qt-based GUI client that wraps the C toxcore library. Understanding this integration is essential for adding PQ support.

## Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Qt UI Layer                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │ FriendWidget│ │ ChatForm    │ │ AboutFriendForm         ││
│  │ (list item) │ │ (chat view) │ │ (friend details)        ││
│  └──────┬──────┘ └──────┬──────┘ └───────────┬─────────────┘│
└─────────┼───────────────┼───────────────────┼───────────────┘
          │               │                   │
┌─────────┼───────────────┼───────────────────┼───────────────┐
│         ▼               ▼                   ▼               │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              Model Layer (src/model/)                   ││
│  │  Friend, Status, IAboutFriend interfaces                ││
│  └──────────────────────┬──────────────────────────────────┘│
└─────────────────────────┼───────────────────────────────────┘
                          │
┌─────────────────────────┼───────────────────────────────────┐
│                         ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              Core Layer (src/core/)                     ││
│  │  Core, ToxId, ToxPk, CoreFile, CoreAV                   ││
│  └──────────────────────┬──────────────────────────────────┘│
│                         │ Qt Signals/Slots                  │
│                         │ Tox Callbacks                     │
└─────────────────────────┼───────────────────────────────────┘
                          │
┌─────────────────────────┼───────────────────────────────────┐
│                         ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              toxcore C Library                          ││
│  │  tox.h, toxav.h, toxencryptsave.h                       ││
│  │  ─────────────────────────────────────────────────────  ││
│  │  c-toxcore-pq additions:                                ││
│  │  • crypto_core_pq.h (ML-KEM primitives)                 ││
│  │  • tox_friend_get_identity_status()                     ││
│  │  • tox_friend_add_pq() (46-byte addresses)              ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

## Core Wrapper (`src/core/core.cpp`)

### Initialization Flow

```cpp
// core.cpp - Simplified flow
Core::Core(QThread* coreThread) {
    // 1. Create Tox options
    Tox_Options* opts = tox_options_new(nullptr);

    // 2. Create Tox instance
    tox = tox_new(opts, &err);

    // 3. Register callbacks
    tox_callback_friend_connection_status(tox, onConnectionStatusChanged);
    tox_callback_friend_request(tox, onFriendRequest);
    tox_callback_friend_message(tox, onFriendMessage);
    // ... more callbacks

    // 4. Size assertions (NEED MODIFICATION FOR PQ)
    assert(ToxId::size == tox_address_size());      // 38 vs 46
    assert(ToxPk::size == tox_public_key_size());   // 32 (unchanged)
}
```

### Callback Pattern

```cpp
// Static callback receives C data, converts to Qt signals
void Core::onConnectionStatusChanged(Tox* tox, uint32_t friendId,
                                      Tox_Connection status, void* vCore) {
    Core* core = static_cast<Core*>(vCore);

    Status::Status friendStatus;
    switch (status) {
    case TOX_CONNECTION_NONE: friendStatus = Status::Offline; break;
    case TOX_CONNECTION_TCP:  friendStatus = Status::Online; break;
    case TOX_CONNECTION_UDP:  friendStatus = Status::Online; break;
    }

    // Emit Qt signal to UI
    emit core->friendStatusChanged(friendId, friendStatus);
}
```

**For PQ, we need to add:**
```cpp
// New callback for PQ identity status
void Core::onFriendIdentityStatusChanged(Tox* tox, uint32_t friendId,
                                          Tox_Connection_Identity status,
                                          void* vCore) {
    Core* core = static_cast<Core*>(vCore);
    emit core->friendIdentityStatusChanged(friendId, status);
}
```

## ToxId Class (`src/core/toxid.cpp`)

### Current Structure (38 bytes)

```cpp
class ToxId {
    static constexpr int size = 38;

    // Layout: [PublicKey:32][NoSpam:4][Checksum:2]
    QByteArray toxId;  // Raw 38 bytes

    ToxPk getPublicKey() const {
        return ToxPk(toxId.left(32));
    }

    uint32_t getNoSpam() const {
        // Bytes 32-35
    }

    uint16_t getChecksum() const {
        // Bytes 36-37
    }

    static bool isToxId(const QString& id) {
        // Check length == 76 hex chars
        // Check format with regex
    }
};
```

### Required PQ Structure (46 bytes)

```cpp
class ToxId {
    // Support both formats
    static constexpr int classicalSize = 38;
    static constexpr int pqSize = 46;

    // Layout (PQ): [PublicKey:32][MLKEMCommit:8][NoSpam:4][Checksum:2]
    QByteArray toxId;
    bool isPqAddress;

    QByteArray getMlkemCommitment() const {
        if (!isPqAddress) return QByteArray();
        return toxId.mid(32, 8);  // Bytes 32-39
    }

    static bool isToxId(const QString& id) {
        // Check length == 76 OR 92 hex chars
        // Validate checksum for detected format
    }
};
```

## Build System (`cmake/Dependencies.cmake`)

### Current toxcore Detection

```cmake
# Lines 156-169
if(WIN32 OR ANDROID OR FULLY_STATIC)
    search_dependency(TOXCORE PACKAGE toxcore LIBRARY toxcore OPTIONAL STATIC_PACKAGE)
else()
    search_dependency(TOXCORE PACKAGE toxcore LIBRARY toxcore OPTIONAL)
    search_dependency(TOXAV PACKAGE toxav LIBRARY toxav OPTIONAL)
    search_dependency(TOXENCRYPTSAVE PACKAGE toxencryptsave LIBRARY toxencryptsave OPTIONAL)
endif()
```

### Required PQ Detection

```cmake
# Add after toxcore detection
if(TOXCORE_FOUND)
    # Check for PQ support by looking for new API
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_LIBRARIES ${TOXCORE_LIBRARIES})
    check_symbol_exists(tox_friend_get_identity_status "tox/tox.h" HAVE_TOX_PQ)

    if(HAVE_TOX_PQ)
        message(STATUS "Found PQ-enabled toxcore")
        add_definitions(-DQTOX_PQ_SUPPORT)
    else()
        message(STATUS "Classical toxcore (no PQ support)")
    endif()
endif()
```

## Signal Flow Example: Friend Connection

```
toxcore                    Core                      UI
   │                        │                        │
   │ tox_callback_*         │                        │
   │───────────────────────>│                        │
   │                        │                        │
   │                        │ emit friendStatusChanged()
   │                        │───────────────────────>│
   │                        │                        │
   │                        │                        │ Update FriendWidget
   │                        │                        │ Update ChatFormHeader
   │                        │                        │
```

**With PQ Extension:**

```
toxcore                    Core                      UI
   │                        │                        │
   │ tox_callback_*         │                        │
   │───────────────────────>│                        │
   │                        │                        │
   │                        │ emit friendStatusChanged()
   │                        │───────────────────────>│
   │                        │                        │
   │ identity_status_cb     │                        │
   │───────────────────────>│                        │
   │                        │ emit friendIdentityStatusChanged()
   │                        │───────────────────────>│
   │                        │                        │
   │                        │                        │ Update PQ indicator
   │                        │                        │ Show shield icon
   │                        │                        │
```

## Key Files Summary

| File | Purpose | PQ Changes Needed |
|------|---------|-------------------|
| `src/core/core.cpp` | Tox wrapper | Add identity callback, fix assertions |
| `src/core/core.h` | Core interface | Add identity signal |
| `src/core/toxid.cpp` | Address handling | Support 46-byte format |
| `src/core/toxid.h` | ToxId class | Add PQ fields |
| `src/model/friend.h` | Friend data | Add identity status field |
| `src/model/status.h` | Status enum | Add PQ status enum |
| `cmake/Dependencies.cmake` | Build config | Detect PQ toxcore |

## Dependencies

qTox requires:
- Qt 5.15+ (GUI framework)
- toxcore (Tox protocol) - **needs PQ-enabled version**
- libsodium (crypto) - **needs ML-KEM support**
- opus (audio codec)
- libvpx (video codec)
- SQLCipher (encrypted database)
- various Qt modules (multimedia, svg, etc.)
