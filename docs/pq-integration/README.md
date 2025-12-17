# pqTox: Post-Quantum Enabled qTox Client

This directory documents the integration of post-quantum cryptography into qTox, creating **pqTox** - a quantum-resistant Tox client.

## Overview

pqTox extends the popular qTox client to leverage the ML-KEM-768 + X25519 hybrid key exchange implemented in c-toxcore-pq. This provides:

- **Quantum-resistant sessions**: All friend communications use hybrid PQ encryption
- **Identity verification**: 46-byte Tox IDs with ML-KEM commitment for PQ identity binding
- **Visual security indicators**: UI shows PQ verification status for each friend
- **Backwards compatibility**: Works with classical Tox clients (falls back gracefully)

## Repository

pqTox is cloned from [TokTok/qTox](https://github.com/TokTok/qTox) to `/home/waterfall/pqTox/`

## Documentation Structure

| Document | Description |
|----------|-------------|
| [01-ARCHITECTURE.md](01-ARCHITECTURE.md) | How qTox integrates with toxcore |
| [02-REQUIRED-CHANGES.md](02-REQUIRED-CHANGES.md) | Detailed code modifications needed |
| [03-IMPLEMENTATION-GUIDE.md](03-IMPLEMENTATION-GUIDE.md) | Step-by-step implementation plan |
| [04-BUILD-GUIDE.md](04-BUILD-GUIDE.md) | Building pqTox with PQ-enabled toxcore |

## Quick Start

```bash
# Build PQ-enabled toxcore first
cd /home/waterfall/c-toxcore-pq
./scripts/build-windows-native.sh  # For Windows
# OR standard cmake build for Linux

# Build pqTox
cd /home/waterfall/pqTox
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/pq-toxcore
make -j$(nproc)
```

## Security Model

### Connection Identity Status

| Status | Description | UI Indicator |
|--------|-------------|--------------|
| `CLASSICAL` | X25519-only session (legacy client) | Yellow shield |
| `PQ_UNVERIFIED` | Hybrid session, no commitment verification | Blue shield |
| `PQ_VERIFIED` | Hybrid session + ML-KEM commitment verified | Green shield |

### Address Formats

| Format | Size | Hex Length | Description |
|--------|------|------------|-------------|
| Classical | 38 bytes | 76 chars | `[X25519_pk:32][nospam:4][checksum:2]` |
| PQ Extended | 46 bytes | 92 chars | `[X25519_pk:32][MLKEM_commit:8][nospam:4][checksum:2]` |

## Key Integration Points

### Core Layer (`src/core/`)
- `core.cpp` - Tox API wrapper, connection callbacks
- `toxid.cpp` - Address validation (needs PQ extension)
- `toxpk.cpp` - Public key handling

### UI Layer
- `aboutfriendform.cpp` - Friend details dialog
- `chatformheader.cpp` - Chat window header
- `friendwidget.cpp` - Friend list item
- `addfriendform.cpp` - Add friend dialog

### Build System
- `cmake/Dependencies.cmake` - Toxcore detection

## Status

| Phase | Status | Description |
|-------|--------|-------------|
| Analysis | ✅ Complete | Codebase exploration done |
| Documentation | 🚧 In Progress | Creating integration docs |
| Implementation | 📋 Planned | Code changes pending |
| Testing | 📋 Planned | Integration tests pending |
| Windows Build | 📋 Planned | Cross-compile setup pending |

## Related Documents

- [c-toxcore-pq Phase 2.5: Identity Commitment](../PHASE-2-HANDSHAKE.md)
- [ML-KEM-768 Crypto Design](../02-CRYPTO-DESIGN.md)
- [Protocol Negotiation](../03-PROTOCOL-NEGOTIATION.md)
