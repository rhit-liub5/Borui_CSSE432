"""Interactive peer entry point."""

from __future__ import annotations

import argparse
import shlex
import threading
from pathlib import Path

import peer_client
from file_manager import FileManager, LocalFile
from peer_server import start_peer_server
from tracker_client import PeerSource, QueryResult, TrackerClient


def print_lines(lines: list[str]) -> None:
    for line in lines:
        print(line)


def register_local_file(tracker: TrackerClient, peer_id: str, info: LocalFile) -> None:
    print_lines(tracker.add_file(peer_id, info.filename, info.filesize))


def choose_source(result: QueryResult, self_id: str) -> PeerSource | None:
    for source in result.peers:
        if source.peer_id != self_id:
            return source
    return result.peers[0] if result.peers else None


def download_whole_file(
    tracker: TrackerClient, manager: FileManager, peer_id: str, filename: str
) -> None:
    result = tracker.query(filename)
    if result is None:
        print(f"NOT_FOUND {filename}")
        return
    source = choose_source(result, peer_id)
    if source is None:
        print("ERROR no source peer")
        return

    data = peer_client.download_file(source.ip, source.port, filename)
    target = manager.write_file(filename, data)
    print_lines(tracker.have(peer_id, filename))
    print(f"downloaded {filename} from {source.peer_id} -> {target}")


def print_help() -> None:
    print(
        "commands:\n"
        "  add <path> [filename]        import and register a local file\n"
        "  query <filename>             ask tracker for sources\n"
        "  list                         list tracker state\n"
        "  files                        list this peer's complete files\n"
        "  download-file <filename>     download whole file from one peer\n"
        "  have <filename>              announce one complete local file\n"
        "  help                         show this help\n"
        "  quit                         stop peer\n"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Run an interactive P2P peer.")
    parser.add_argument("--peer-id", required=True)
    parser.add_argument("--tracker-host", default="127.0.0.1")
    parser.add_argument("--tracker-port", type=int, default=9000)
    parser.add_argument("--listen-host", default="127.0.0.1")
    parser.add_argument("--listen-port", type=int, required=True)
    parser.add_argument("--shared-dir", default=None)
    parser.add_argument("--download-dir", default=None)
    args = parser.parse_args()

    base = Path("peer_data") / args.peer_id
    manager = FileManager(
        args.shared_dir or str(base / "shared"),
        args.download_dir or str(base / "downloads"),
    )
    tracker = TrackerClient(args.tracker_host, args.tracker_port)

    server = start_peer_server(args.listen_host, args.listen_port, manager)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    print(f"[peer {args.peer_id}] serving on {args.listen_host}:{args.listen_port}")
    print_lines(tracker.register(args.peer_id, args.listen_host, args.listen_port))

    print_help()
    while True:
        try:
            raw = input(f"{args.peer_id}> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not raw:
            continue
        try:
            parts = shlex.split(raw)
        except ValueError as exc:
            print(f"ERROR {exc}")
            continue

        command = parts[0].lower()
        try:
            if command in {"quit", "exit"}:
                break
            if command == "help":
                print_help()
            elif command == "add" and len(parts) in {2, 3}:
                info = manager.import_file(parts[1], parts[2] if len(parts) == 3 else None)
                register_local_file(tracker, args.peer_id, info)
            elif command == "query" and len(parts) == 2:
                result = tracker.query(parts[1])
                print(result if result is not None else f"NOT_FOUND {parts[1]}")
            elif command == "list":
                print_lines(tracker.list())
            elif command == "files":
                for info in manager.list_complete_files():
                    print(f"{info.filename} {info.filesize} bytes")
            elif command == "download-file" and len(parts) == 2:
                download_whole_file(tracker, manager, args.peer_id, parts[1])
            elif command == "have" and len(parts) == 2:
                info = manager.local_file(parts[1])
                print_lines(tracker.have(args.peer_id, info.filename))
            else:
                print("ERROR unknown command; type help")
        except Exception as exc:
            print(f"ERROR {exc}")

    server.shutdown()
    server.server_close()


if __name__ == "__main__":
    main()
