"""Peer-to-peer download client."""

from __future__ import annotations

import socket
import shlex

from protocol import ProtocolError, read_line, recv_exact, send_line


def download_file(host: str, port: int, filename: str) -> bytes:
    with socket.create_connection((host, port), timeout=20.0) as sock:
        send_line(sock, f"GET_FILE {shlex.quote(filename)}")
        header = read_line(sock).split()
        if not header:
            raise ProtocolError("empty peer response")
        if header[0] == "ERROR":
            raise ProtocolError(" ".join(header[1:]))
        if len(header) != 2 or header[0] != "OK":
            raise ProtocolError(f"unexpected peer response: {' '.join(header)}")
        return recv_exact(sock, int(header[1]))


def download_piece(host: str, port: int, filename: str, piece_id: int) -> bytes:
    with socket.create_connection((host, port), timeout=20.0) as sock:
        send_line(sock, f"GET_PIECE {shlex.quote(filename)} {piece_id}")
        header = shlex.split(read_line(sock))
        if not header:
            raise ProtocolError("empty peer response")
        if header[0] == "ERROR":
            raise ProtocolError(" ".join(header[1:]))
        if len(header) != 4 or header[0] != "PIECE":
            raise ProtocolError(f"unexpected peer response: {' '.join(header)}")
        returned_piece = int(header[2])
        if returned_piece != piece_id:
            raise ProtocolError(f"expected piece {piece_id}, got {returned_piece}")
        return recv_exact(sock, int(header[3]))
