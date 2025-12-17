# pqTox

**Post-Quantum Tox Client** - A fork of [qTox](https://github.com/TokTok/qTox) with ML-KEM-768 hybrid encryption support.

<p align="center">
<img src="https://qtox.github.io/assets/imgs/logo_head.png" alt="pqTox" />
</p>

---

<p align="center">
<a href="https://github.com/moscowchill/pqTox/blob/master/LICENSE">
<img src="https://img.shields.io/badge/license-GPLv3%2B-blue.svg" alt="GPLv3+" />
</a>
</p>

---

<p align="center"><b>
pqTox is a quantum-resistant chat, voice, video, and file transfer client using
the Tox protocol with ML-KEM-768 + X25519 hybrid key exchange.
</b></p>

## What's Different from qTox?

pqTox adds **post-quantum cryptography** to protect against future quantum computer attacks:

| Feature | qTox | pqTox |
|---------|------|-------|
| Key Exchange | X25519 | ML-KEM-768 + X25519 hybrid |
| Address Size | 38 bytes (76 hex) | 46 bytes (92 hex) |
| Identity Verification | None | ML-KEM commitment |
| Quantum Resistance | No | Yes |

## Security Model

### Connection Identity Status

| Status | Icon | Description |
|--------|------|-------------|
| **PQ Verified** | Green Shield | Hybrid encryption + identity verified |
| **PQ Unverified** | Blue Shield | Hybrid encryption, no commitment check |
| **Classical** | Yellow Shield | X25519 only (legacy peer) |

### Backwards Compatibility

pqTox is fully backwards compatible with classical Tox clients:
- Can add friends using 76-char classical addresses
- Falls back to X25519 when peer doesn't support PQ
- Shows security status so users know their protection level

## Building

### Prerequisites

pqTox requires **PQ-enabled toxcore** from [c-toxcore-pq](https://github.com/moscowchill/c-toxcore-pq).

```bash
# Build PQ-enabled toxcore first
git clone https://github.com/moscowchill/c-toxcore-pq.git
cd c-toxcore-pq
mkdir _build && cd _build
cmake .. -DBUILD_TOXAV=ON
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Build pqTox

```bash
git clone https://github.com/moscowchill/pqTox.git
cd pqTox
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
make -j$(nproc)
```

### Dependencies

- Qt 6.4+
- PQ-enabled toxcore (c-toxcore-pq)
- libsodium (with ML-KEM support)
- FFmpeg, Opus, VPX
- SQLCipher

See [docs/pq-integration/04-BUILD-GUIDE.md](docs/pq-integration/04-BUILD-GUIDE.md) for detailed instructions.

## Documentation

| Document | Description |
|----------|-------------|
| [Architecture](docs/pq-integration/01-ARCHITECTURE.md) | How pqTox integrates with toxcore |
| [Required Changes](docs/pq-integration/02-REQUIRED-CHANGES.md) | Code modifications for PQ support |
| [Implementation Guide](docs/pq-integration/03-IMPLEMENTATION-GUIDE.md) | Step-by-step implementation |
| [Build Guide](docs/pq-integration/04-BUILD-GUIDE.md) | Build instructions |

## Credits

pqTox is based on [qTox](https://github.com/TokTok/qTox) by the TokTok team, which was originally developed by:

- [anthonybilinski](https://github.com/anthonybilinski)
- [Diadlo](https://github.com/Diadlo)
- [sudden6](https://github.com/sudden6)
- [nurupo](https://github.com/nurupo)
- [tux3](https://github.com/tux3)
- [zetok](https://github.com/zetok)
- [iphydf](https://github.com/iphydf)

Post-quantum cryptography implementation by [moscowchill](https://github.com/moscowchill).

## License

GPLv3+ - See [LICENSE](LICENSE) for details.

## Related Projects

- [c-toxcore-pq](https://github.com/moscowchill/c-toxcore-pq) - PQ-enabled Tox protocol library
- [TokTok/qTox](https://github.com/TokTok/qTox) - Original qTox client
- [TokTok/c-toxcore](https://github.com/TokTok/c-toxcore) - Original toxcore library
