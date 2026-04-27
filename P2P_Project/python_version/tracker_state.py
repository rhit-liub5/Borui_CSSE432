"""Thread-safe in-memory tracker state."""

from __future__ import annotations

from dataclasses import dataclass, field
from threading import RLock


@dataclass(frozen=True)
class PeerInfo:
    peer_id: str
    ip: str
    port: int


@dataclass
class FileInfo:
    filename: str
    filesize: int
    piece_size: int
    piece_count: int
    pieces: dict[int, set[str]] = field(default_factory=dict)


class TrackerState:
    def __init__(self) -> None:
        self._lock = RLock()
        self.peers: dict[str, PeerInfo] = {}
        self.files: dict[str, FileInfo] = {}

    def register_peer(self, peer_id: str, ip: str, port: int) -> None:
        with self._lock:
            self.peers[peer_id] = PeerInfo(peer_id, ip, int(port))

    def add_file(
        self,
        peer_id: str,
        filename: str,
        filesize: int,
        piece_size: int,
        piece_count: int,
    ) -> None:
        with self._lock:
            info = self.files.get(filename)
            if info is None:
                info = FileInfo(filename, int(filesize), int(piece_size), int(piece_count))
                self.files[filename] = info
            else:
                info.filesize = int(filesize)
                info.piece_size = int(piece_size)
                info.piece_count = int(piece_count)

            for piece_id in range(info.piece_count):
                info.pieces.setdefault(piece_id, set()).add(peer_id)

    def have_piece(self, peer_id: str, filename: str, piece_id: int) -> None:
        with self._lock:
            if filename not in self.files:
                raise KeyError(f"unknown file: {filename}")
            info = self.files[filename]
            piece = int(piece_id)
            if piece < 0 or piece >= info.piece_count:
                raise ValueError(f"piece_id out of range: {piece}")
            info.pieces.setdefault(piece, set()).add(peer_id)

    def query(self, filename: str) -> tuple[FileInfo | None, list[tuple[PeerInfo, list[int]]]]:
        with self._lock:
            info = self.files.get(filename)
            if info is None:
                return None, []

            peer_pieces: dict[str, list[int]] = {}
            for piece_id, owners in info.pieces.items():
                for peer_id in owners:
                    peer_pieces.setdefault(peer_id, []).append(piece_id)

            rows: list[tuple[PeerInfo, list[int]]] = []
            for peer_id, pieces in sorted(peer_pieces.items()):
                peer = self.peers.get(peer_id)
                if peer is not None:
                    rows.append((peer, sorted(pieces)))
            return info, rows

    def list_files(self) -> list[FileInfo]:
        with self._lock:
            return list(sorted(self.files.values(), key=lambda item: item.filename))

    def list_peers(self) -> list[PeerInfo]:
        with self._lock:
            return list(sorted(self.peers.values(), key=lambda item: item.peer_id))
