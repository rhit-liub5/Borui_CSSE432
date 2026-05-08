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

        manager_a = FileManager(root / "A" / "shared", root / "A" / "downloads")
        manager_b = FileManager(root / "B" / "shared", root / "B" / "downloads")

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
        print(tracker.add_file("A", info_a.filename, info_a.filesize)[0])

        result = tracker.query("demo.txt")
        assert result is not None
        print(f"query: {result.filename}, size={result.filesize}, peers={len(result.peers)}")

        data = peer_client.download_file("127.0.0.1", port_a, "demo.txt")
        downloaded = manager_b.write_file("demo.txt", data)
        assert downloaded.read_text(encoding="utf-8") == source.read_text(encoding="utf-8")
        print(tracker.have("B", "demo.txt")[0])
        print(f"downloaded: {downloaded}")

        peer_a.shutdown()
        peer_b.shutdown()
        tracker_server.shutdown()
        peer_a.server_close()
        peer_b.server_close()
        tracker_server.server_close()


if __name__ == "__main__":
    main()
