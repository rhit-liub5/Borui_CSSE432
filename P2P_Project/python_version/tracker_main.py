"""Tracker server entry point."""

from __future__ import annotations

import argparse
import socketserver

from protocol import ProtocolError, read_line, send_line
from tracker_handler import handle_command
from tracker_state import TrackerState


class TrackerRequestHandler(socketserver.BaseRequestHandler):
    state: TrackerState

    def handle(self) -> None:
        try:
            line = read_line(self.request)
            print(f"[tracker] {self.client_address[0]}:{self.client_address[1]} -> {line}")
            for response in handle_command(self.state, line):
                send_line(self.request, response)
            send_line(self.request, "END")
        except ProtocolError as exc:
            send_line(self.request, f"ERROR {exc}")
            send_line(self.request, "END")


class ThreadedTracker(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the P2P tracker server.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9000)
    args = parser.parse_args()

    TrackerRequestHandler.state = TrackerState()
    with ThreadedTracker((args.host, args.port), TrackerRequestHandler) as server:
        print(f"[tracker] listening on {args.host}:{args.port}")
        server.serve_forever()


if __name__ == "__main__":
    main()
