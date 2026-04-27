"""Command handling for peer-to-tracker messages."""

from __future__ import annotations

import shlex

from tracker_state import TrackerState


def handle_command(state: TrackerState, line: str) -> list[str]:
    try:
        parts = shlex.split(line)
    except ValueError as exc:
        return [f"ERROR bad command: {exc}"]

    if not parts:
        return ["ERROR empty command"]

    command = parts[0].upper()
    try:
        if command == "REGISTER" and len(parts) == 4:
            peer_id, ip, port = parts[1], parts[2], int(parts[3])
            state.register_peer(peer_id, ip, port)
            return [f"OK REGISTERED {peer_id}"]

        if command == "ADD_FILE" and len(parts) == 6:
            peer_id, filename = parts[1], parts[2]
            filesize, piece_size, piece_count = map(int, parts[3:6])
            state.add_file(peer_id, filename, filesize, piece_size, piece_count)
            return [f"OK FILE_ADDED {filename}"]

        if command == "HAVE" and len(parts) == 4:
            peer_id, filename, piece_id = parts[1], parts[2], int(parts[3])
            state.have_piece(peer_id, filename, piece_id)
            return [f"OK HAVE {filename} {piece_id}"]

        if command == "QUERY" and len(parts) == 2:
            filename = parts[1]
            info, rows = state.query(filename)
            if info is None:
                return [f"NOT_FOUND {filename}"]
            response = [
                f"PEERS {info.filename} {info.filesize} {info.piece_size} {info.piece_count}"
            ]
            for peer, pieces in rows:
                piece_list = ",".join(str(piece) for piece in pieces)
                response.append(f"{peer.peer_id} {peer.ip} {peer.port} {piece_list}")
            return response

        if command == "LIST":
            files = state.list_files()
            peers = state.list_peers()
            response = [f"PEER_COUNT {len(peers)}", f"FILE_COUNT {len(files)}"]
            response.extend(
                f"FILE {info.filename} {info.filesize} {info.piece_size} {info.piece_count}"
                for info in files
            )
            return response
    except (KeyError, ValueError) as exc:
        return [f"ERROR {exc}"]

    return [f"ERROR unsupported or malformed command: {line}"]
