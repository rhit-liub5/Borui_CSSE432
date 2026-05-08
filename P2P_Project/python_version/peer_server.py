"""Upload server that lets other peers request complete files."""

from __future__ import annotations

import shlex
import socketserver

from file_manager import FileManager
from protocol import ProtocolError, read_line, send_line


class PeerRequestHandler(socketserver.BaseRequestHandler):
    def handle(self) -> None:
        try:
            line = read_line(self.request)
            parts = shlex.split(line)
            if not parts:
                send_line(self.request, "ERROR empty command")
                return

            command = parts[0].upper()
            if command == "GET_FILE" and len(parts) == 2:
                data = self.file_manager.read_file(parts[1])
                send_line(self.request, f"OK {len(data)}")
                self.request.sendall(data)
                return

            send_line(self.request, f"ERROR unsupported command: {line}")
        except (FileNotFoundError, ValueError, ProtocolError) as exc:
            send_line(self.request, f"ERROR {exc}")


class ThreadedPeerServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True


def start_peer_server(host: str, port: int, file_manager: FileManager) -> ThreadedPeerServer:
    class BoundPeerRequestHandler(PeerRequestHandler):
        pass

    BoundPeerRequestHandler.file_manager = file_manager
    server = ThreadedPeerServer((host, port), BoundPeerRequestHandler)
    return server
