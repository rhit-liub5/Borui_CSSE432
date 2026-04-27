# Python P2P Version

## Run Demo Manually

Use three terminals from the repository root.

Terminal 1:

```bash
python3 P2P_Project/python_version/tracker_main.py --host 127.0.0.1 --port 9000
```

Terminal 2:

```bash
python3 P2P_Project/python_version/peer_main.py --peer-id A --listen-port 9101
```

Inside peer A:

```text
add Lab02/Server/store/hello.txt test.txt
```

Terminal 3:

```bash
python3 P2P_Project/python_version/peer_main.py --peer-id B --listen-port 9102
```

Inside peer B:

```text
query test.txt
download-pieces test.txt
files
```

## One-Command Local Demo

```bash
python3 P2P_Project/python_version/demo_local.py
```

## Modules

- `tracker_main.py`: starts the tracker server.
- `tracker_handler.py`: parses tracker commands.
- `tracker_state.py`: stores peers, files, and piece ownership.
- `peer_main.py`: interactive peer program.
- `peer_server.py`: upload server for peer-to-peer requests.
- `peer_client.py`: download client for whole files or pieces.
- `file_manager.py`: imports files, reads pieces, writes parts, and assembles downloads.
- `tracker_client.py`: wrapper for peer-to-tracker commands.
- `protocol.py`: shared socket helpers.
