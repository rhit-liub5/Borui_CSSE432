# Simplified BitTorrent P2P Project

This project is split into two implementations:

- `python_version/`: Python implementation, useful for quick testing and readable protocol flow.
- `cpp_version/`: C++ implementation, intended as the main course-project version.

Both versions implement the same simplified BitTorrent-style design:

- Tracker records online peers, files, and piece ownership.
- Peers register with the tracker.
- Peers query file sources from the tracker.
- Peers serve files or pieces to other peers.
- Downloaded pieces are saved and assembled into the complete file.
- Peers announce `HAVE` after receiving pieces so they can become upload sources.

## Recommended Version

Use the C++ version for submission/demo:

```bash
cd P2P_Project/cpp_version
make
```

See [cpp_version/README.md](cpp_version/README.md) for C++ commands.

## Python Version

See [python_version/README.md](python_version/README.md) for Python commands.
