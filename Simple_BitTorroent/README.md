# Simple BitTorrent

This project is a small BitTorrent-style peer-to-peer file sharing demo written in Python. It has one tracker and multiple peers running on localhost.

## Overview

- `tracker.py` keeps track of which peer has which file.
- Each peer runs a small server so other peers can download files from its shared folder.
- When a peer downloads a file successfully, it sends an update to the tracker so it can also become a source for that file.

## Project Structure

```text
Simple_BitTorroent/
├── tracker.py
├── protocol.py
├── peer1/
│   ├── peer.py
│   ├── protocol.py
│   └── share/
│       └── hello.txt
├── peer2/
│   ├── peer.py
│   ├── protocol.py
│   └── share/
│       └── hi.txt
└── peer3/
    ├── peer.py
    ├── protocol.py
    └── share/
```

## Requirements

- Python 3
- No external Python packages are required.

## How to Run

Open separate terminal windows for the tracker and each peer.

### 1. Start the tracker

From the project root:

```bash
cd Simple_BitTorroent
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

Peer 1 starts with `hello.txt` in its shared folder.

### 3. Start peer 2

In another terminal:

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

You can also try:

```text
hi.txt
```

To stop a peer, type:

```text
quit
```

## Protocol Messages

The project uses simple text-based messages over TCP sockets.

```text
REGISTER <peer_name> <ip> <port> <filename1> <filename2> ...
QUERY <filename>
FOUND <peer_name> <ip> <port>
GET <filename>
UPDATE <peer_name> <ip> <port> <filename>
OK REGISTERED
OK UPDATED
OK <filesize>
NOT_FOUND
END
ERROR <message>
```

## Notes

- The demo is designed to run locally on `127.0.0.1`.
- The tracker must be started before the peers.
- Each peer should use a different port.
- Downloaded files are saved into that peer's `share` folder.
- The folder name is currently `Simple_BitTorroent`, so use that exact spelling when running commands.
