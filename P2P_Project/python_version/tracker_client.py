"""Client wrapper for peer-to-tracker commands."""

from __future__ import annotations

import shlex
from dataclasses import dataclass

from protocol import request_lines


@dataclass(frozen=True)
class PeerSource:
    peer_id: str
    ip: str
    port: int
    pieces: set[int]


@dataclass(frozen=True)
class QueryResult:
    filename: str
    filesize: int
    piece_size: int
    piece_count: int
    peers: list[PeerSource]


class TrackerClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port

    def _send(self, command: str) -> list[str]:
        return request_lines(self.host, self.port, command)

    def register(self, peer_id: str, ip: str, port: int) -> list[str]:
        return self._send(f"REGISTER {shlex.quote(peer_id)} {ip} {port}")

    def add_file(
        self, peer_id: str, filename: str, filesize: int, piece_size: int, piece_count: int
    ) -> list[str]:
        return self._send(
            "ADD_FILE "
            f"{shlex.quote(peer_id)} {shlex.quote(filename)} {filesize} {piece_size} {piece_count}"
        )

    def have(self, peer_id: str, filename: str, piece_id: int) -> list[str]:
        return self._send(f"HAVE {shlex.quote(peer_id)} {shlex.quote(filename)} {piece_id}")

    def list(self) -> list[str]:
        return self._send("LIST")

    def query(self, filename: str) -> QueryResult | None:
        lines = self._send(f"QUERY {shlex.quote(filename)}")
        if not lines or lines[0].startswith("NOT_FOUND"):
            return None
        header = shlex.split(lines[0])
        if len(header) != 5 or header[0] != "PEERS":
            raise ValueError(f"unexpected tracker response: {lines}")

        peers: list[PeerSource] = []
        for line in lines[1:]:
            parts = shlex.split(line)
            if len(parts) != 4:
                continue
            pieces = {int(item) for item in parts[3].split(",") if item != ""}
            peers.append(PeerSource(parts[0], parts[1], int(parts[2]), pieces))

        return QueryResult(header[1], int(header[2]), int(header[3]), int(header[4]), peers)
