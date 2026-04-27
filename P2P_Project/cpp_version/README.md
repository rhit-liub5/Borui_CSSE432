# C++ P2P Version

## Build

```bash
make
```

This creates:

- `tracker`
- `peer`

## Run Demo Manually

Use three terminals from `P2P_Project/cpp_version`.

Terminal 1:

```bash
./tracker 9000 127.0.0.1
```

Terminal 2:

```bash
./peer A 9101 127.0.0.1 9000 127.0.0.1
```

Inside peer A:

```text
add ../../Lab02/Server/store/hello.txt test.txt
```

Terminal 3:

```bash
./peer B 9102 127.0.0.1 9000 127.0.0.1
```

Inside peer B:

```text
query test.txt
download-pieces test.txt
files
```

## Modules

- `tracker.cpp`: starts the tracker server and handles tracker commands.
- `tracker_state.hpp/.cpp`: stores peers, files, and piece ownership.
- `peer.cpp`: interactive peer program, upload server, tracker client, and download client.
- `file_manager.hpp/.cpp`: imports files, reads pieces, writes parts, and assembles downloads.
- `common.hpp/.cpp`: shared socket and parsing helpers.
- `Makefile`: builds `tracker` and `peer`.

## Supported Commands

Peer interactive commands:

```text
add <path> [filename]
query <filename>
list
files
download-file <filename>
download-pieces <filename>
have <filename>
help
quit
```

## Protocol

Tracker commands:

```text
REGISTER <peer_id> <ip> <port>
ADD_FILE <peer_id> <filename> <filesize> <piece_size> <piece_count>
HAVE <peer_id> <filename> <piece_id>
QUERY <filename>
LIST
```

Peer-to-peer commands:

```text
GET_FILE <filename>
GET_PIECE <filename> <piece_id>
```
