"""Interactive peer entry point."""

from __future__ import annotations

import argparse
import shlex
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import peer_client
from file_manager import DEFAULT_PIECE_SIZE, FileManager, LocalFile
from peer_server import start_peer_server
from tracker_client import PeerSource, QueryResult, TrackerClient


def print_lines(lines: list[str]) -> None:
    for line in lines:
        print(line)


def register_local_file(tracker: TrackerClient, peer_id: str, info: LocalFile) -> None:
    print_lines(
        tracker.add_file(peer_id, info.filename, info.filesize, info.piece_size, info.piece_count)
    )


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
    target = manager.download_dir / filename
    target.write_bytes(data)
    info = manager.local_file(filename)
    register_local_file(tracker, peer_id, info)
    print(f"downloaded {filename} from {source.peer_id} -> {target}")


def source_for_piece(result: QueryResult, self_id: str, piece_id: int) -> PeerSource | None:
    for source in result.peers:
        if source.peer_id != self_id and piece_id in source.pieces:
            return source
    for source in result.peers:
        if piece_id in source.pieces:
            return source
    return None


def download_pieces(
    tracker: TrackerClient, manager: FileManager, peer_id: str, filename: str, workers: int
) -> None:
    result = tracker.query(filename)
    if result is None:
        print(f"NOT_FOUND {filename}")
        return

    have = set(manager.complete_piece_ids(filename, result.piece_count))
    missing = [piece_id for piece_id in range(result.piece_count) if piece_id not in have]
    if not missing:
        print(f"already complete: {filename}")
        return

    jobs = []
    with ThreadPoolExecutor(max_workers=max(1, workers)) as pool:
        for piece_id in missing:
            source = source_for_piece(result, peer_id, piece_id)
            if source is None:
                print(f"no source for piece {piece_id}")
                continue
            jobs.append(
                pool.submit(peer_client.download_piece, source.ip, source.port, filename, piece_id)
            )
            jobs[-1].piece_id = piece_id  # type: ignore[attr-defined]
            jobs[-1].source_id = source.peer_id  # type: ignore[attr-defined]

        for job in as_completed(jobs):
            piece_id = job.piece_id  # type: ignore[attr-defined]
            source_id = job.source_id  # type: ignore[attr-defined]
            data = job.result()
            manager.write_piece(filename, piece_id, data)
            print_lines(tracker.have(peer_id, filename, piece_id))
            print(f"piece {piece_id} downloaded from {source_id}")

    have = set(manager.complete_piece_ids(filename, result.piece_count))
    if len(have) == result.piece_count:
        target = manager.assemble_file(filename, result.piece_count)
        info = manager.local_file(filename)
        register_local_file(tracker, peer_id, info)
        print(f"assembled {filename} -> {target}")
    else:
        print(f"partial download: {len(have)}/{result.piece_count} pieces")


def print_help() -> None:
    print(
        "commands:\n"
        "  add <path> [filename]        import and register a local file\n"
        "  query <filename>             ask tracker for sources\n"
        "  list                         list tracker state\n"
        "  files                        list this peer's complete files\n"
        "  download-file <filename>     download whole file from one peer\n"
        "  download-pieces <filename>   download pieces, using multiple peers when available\n"
        "  have <filename>              announce all complete local pieces\n"
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
    parser.add_argument("--piece-size", type=int, default=DEFAULT_PIECE_SIZE)
    parser.add_argument("--workers", type=int, default=4)
    args = parser.parse_args()

    base = Path("peer_data") / args.peer_id
    manager = FileManager(
        args.shared_dir or str(base / "shared"),
        args.download_dir or str(base / "downloads"),
        args.piece_size,
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
                    print(f"{info.filename} {info.filesize} bytes {info.piece_count} pieces")
            elif command == "download-file" and len(parts) == 2:
                download_whole_file(tracker, manager, args.peer_id, parts[1])
            elif command == "download-pieces" and len(parts) == 2:
                download_pieces(tracker, manager, args.peer_id, parts[1], args.workers)
            elif command == "have" and len(parts) == 2:
                info = manager.local_file(parts[1])
                for piece_id in range(info.piece_count):
                    print_lines(tracker.have(args.peer_id, info.filename, piece_id))
            else:
                print("ERROR unknown command; type help")
        except Exception as exc:
            print(f"ERROR {exc}")

    server.shutdown()
    server.server_close()


if __name__ == "__main__":
    main()
