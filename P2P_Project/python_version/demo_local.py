"""Run a local end-to-end demo without interactive terminals."""

from __future__ import annotations

import tempfile
import threading
from pathlib import Path

import peer_client
from file_manager import FileManager
from peer_server import start_peer_server
from tracker_client import TrackerClient
from tracker_main import ThreadedTracker, TrackerRequestHandler
from tracker_state import TrackerState


def serve(server) -> threading.Thread:
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return thread


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)

        TrackerRequestHandler.state = TrackerState()
        tracker_server = ThreadedTracker(("127.0.0.1", 0), TrackerRequestHandler)
        serve(tracker_server)
        tracker_port = tracker_server.server_address[1]
        tracker = TrackerClient("127.0.0.1", tracker_port)

        manager_a = FileManager(root / "A" / "shared", root / "A" / "downloads", piece_size=8)
        manager_b = FileManager(root / "B" / "shared", root / "B" / "downloads", piece_size=8)

        source = root / "source.txt"
        source.write_text("abcdefghijklmnopqrstuvwxyz\n", encoding="utf-8")
        info_a = manager_a.import_file(str(source), "demo.txt")

        peer_a = start_peer_server("127.0.0.1", 0, manager_a)
        peer_b = start_peer_server("127.0.0.1", 0, manager_b)
        serve(peer_a)
        serve(peer_b)
        port_a = peer_a.server_address[1]
        port_b = peer_b.server_address[1]

        print(tracker.register("A", "127.0.0.1", port_a)[0])
        print(tracker.register("B", "127.0.0.1", port_b)[0])
        print(tracker.add_file("A", info_a.filename, info_a.filesize, info_a.piece_size, info_a.piece_count)[0])

        result = tracker.query("demo.txt")
        assert result is not None
        print(f"query: {result.filename}, pieces={result.piece_count}, peers={len(result.peers)}")

        for piece_id in range(result.piece_count):
            data = peer_client.download_piece("127.0.0.1", port_a, "demo.txt", piece_id)
            manager_b.write_piece("demo.txt", piece_id, data)
            print(tracker.have("B", "demo.txt", piece_id)[0])

        assembled = manager_b.assemble_file("demo.txt", result.piece_count)
        assert assembled.read_text(encoding="utf-8") == source.read_text(encoding="utf-8")
        info_b = manager_b.local_file("demo.txt")
        print(tracker.add_file("B", info_b.filename, info_b.filesize, info_b.piece_size, info_b.piece_count)[0])
        print(f"assembled: {assembled}")

        peer_a.shutdown()
        peer_b.shutdown()
        tracker_server.shutdown()
        peer_a.server_close()
        peer_b.server_close()
        tracker_server.server_close()


if __name__ == "__main__":
    main()
