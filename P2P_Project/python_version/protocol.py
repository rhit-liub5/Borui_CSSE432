"""Shared socket helpers for the simplified BitTorrent project."""

from __future__ import annotations

import socket


ENCODING = "utf-8"


class ProtocolError(Exception):
    """Raised when a peer sends malformed or incomplete data."""


def send_line(sock: socket.socket, line: str) -> None:
    sock.sendall((line.rstrip("\n") + "\n").encode(ENCODING))


def read_line(sock: socket.socket) -> str:
    chunks: list[bytes] = []
    while True:
        char = sock.recv(1)
        if not char:
            if chunks:
                break
            raise ProtocolError("connection closed while reading line")
        if char == b"\n":
            break
        chunks.append(char)
    return b"".join(chunks).decode(ENCODING).rstrip("\r")


def recv_exact(sock: socket.socket, size: int) -> bytes:
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise ProtocolError("connection closed while reading payload")
        data.extend(chunk)
    return bytes(data)


def request_lines(host: str, port: int, request: str, timeout: float = 10.0) -> list[str]:
    with socket.create_connection((host, port), timeout=timeout) as sock:
        send_line(sock, request)
        lines: list[str] = []
        while True:
            line = read_line(sock)
            if line == "END":
                return lines
            lines.append(line)
