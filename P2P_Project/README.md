# Simplified BitTorrent P2P Project

This project is split into two implementations:

- `python_version/`: Python implementation, useful for quick testing and readable protocol flow.
- `cpp_version/`: C++ implementation, intended as the main course-project version.

The Python version now uses a simplified full-file sharing design:

- Tracker records online peers, files, and full-file ownership.
- Peers register with the tracker.
- Peers query file sources from the tracker.
- Peers serve complete files to other peers.
- Downloaders save complete files directly.
- Peers announce `HAVE` after downloading a full file so they can become upload sources.

The C++ version still follows the original piece-based course-project design.

## Recommended Version

Use the C++ version for submission/demo:

```bash
cd P2P_Project/cpp_version
make
```

See [cpp_version/README.md](cpp_version/README.md) for C++ commands.

## Python Version

See [python_version/README.md](python_version/README.md) for Python commands.
