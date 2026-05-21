# Simple BitTorrent

This project is a small BitTorrent-style peer-to-peer file sharing demo written in Python. It has one tracker and multiple peers running on localhost.

## How to Run

Open separate terminal windows for the tracker and each peer.

### 1. Start the tracker

From the project root:

```bash
python3 tracker.py
```

The tracker listens on:

```text
127.0.0.1:9000
```

### 2. Start peer 1

In a new terminal:

```bash
cd Simple_BitTorroent/peer1
python3 peer.py peer1 5001 share
```

### 3. Start peer 2

```bash
cd Simple_BitTorroent/peer2
python3 peer.py peer2 5002 share
```

Peer 2 starts with `hi.txt` in its shared folder.

### 4. Start peer 3

In another terminal:

```bash
cd Simple_BitTorroent/peer3
python3 peer.py peer3 5003 share
```

Peer 3 starts with an empty shared folder.

## Example Usage

After the peers are running, type a filename into any peer terminal to download it.

For example, in the peer 3 terminal:

```text
hello.txt
```

Peer 3 will:

1. Query the tracker for peers that have `hello.txt`.
2. Download the file from peer 1.
3. Save it into `peer3/share/hello.txt`.
4. Update the tracker so other peers know peer 3 also has the file.

## Notes

- The demo is designed to run locally on `127.0.0.1`.
- The tracker must be started before the peers.
- Each peer should use a different port.
- Downloaded files are saved into that peer's `share` folder.