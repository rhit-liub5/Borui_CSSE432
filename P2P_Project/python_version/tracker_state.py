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
    owners: set[str] = field(default_factory=set)


class TrackerState:
    def __init__(self) -> None:
        self._lock = RLock()
        self.peers: dict[str, PeerInfo] = {}
        self.files: dict[str, FileInfo] = {}

    def register_peer(self, peer_id: str, ip: str, port: int) -> None:
        with self._lock:
            self.peers[peer_id] = PeerInfo(peer_id, ip, int(port))

    def add_file(self, peer_id: str, filename: str, filesize: int) -> None:
        with self._lock:
            info = self.files.get(filename)
            if info is None:
                info = FileInfo(filename, int(filesize))
                self.files[filename] = info
            else:
                info.filesize = int(filesize)

            info.owners.add(peer_id)

    def have_file(self, peer_id: str, filename: str) -> None:
        with self._lock:
            if filename not in self.files:
                raise KeyError(f"unknown file: {filename}")
            self.files[filename].owners.add(peer_id)

    def query(self, filename: str) -> tuple[FileInfo | None, list[PeerInfo]]:
        with self._lock:
            info = self.files.get(filename)
            if info is None:
                return None, []

            owners: list[PeerInfo] = []
            for peer_id in sorted(info.owners):
                peer = self.peers.get(peer_id)
                if peer is not None:
                    owners.append(peer)
            return info, owners

    def list_files(self) -> list[FileInfo]:
        with self._lock:
            return list(sorted(self.files.values(), key=lambda item: item.filename))

    def list_peers(self) -> list[PeerInfo]:
        with self._lock:
            return list(sorted(self.peers.values(), key=lambda item: item.peer_id))
