"""Local file and piece management for peers."""

from __future__ import annotations

import math
import shutil
from dataclasses import dataclass
from pathlib import Path


DEFAULT_PIECE_SIZE = 256 * 1024


@dataclass(frozen=True)
class LocalFile:
    filename: str
    path: Path
    filesize: int
    piece_size: int
    piece_count: int


class FileManager:
    def __init__(self, shared_dir: str, download_dir: str, piece_size: int = DEFAULT_PIECE_SIZE):
        self.shared_dir = Path(shared_dir)
        self.download_dir = Path(download_dir)
        self.piece_size = piece_size
        self.shared_dir.mkdir(parents=True, exist_ok=True)
        self.download_dir.mkdir(parents=True, exist_ok=True)
        self.parts_dir = self.download_dir / ".parts"
        self.parts_dir.mkdir(parents=True, exist_ok=True)

    def local_file(self, filename: str) -> LocalFile:
        path = self.shared_dir / filename
        if not path.exists():
            path = self.download_dir / filename
        if not path.exists():
            raise FileNotFoundError(filename)
        filesize = path.stat().st_size
        piece_count = max(1, math.ceil(filesize / self.piece_size))
        return LocalFile(filename, path, filesize, self.piece_size, piece_count)

    def import_file(self, source: str, filename: str | None = None) -> LocalFile:
        source_path = Path(source)
        target = self.shared_dir / (filename or source_path.name)
        if source_path.resolve() != target.resolve():
            shutil.copyfile(source_path, target)
        return self.local_file(target.name)

    def list_complete_files(self) -> list[LocalFile]:
        files: list[LocalFile] = []
        for path in sorted(self.shared_dir.iterdir()):
            if path.is_file():
                files.append(self.local_file(path.name))
        for path in sorted(self.download_dir.iterdir()):
            if path.is_file() and not (self.shared_dir / path.name).exists():
                files.append(self.local_file(path.name))
        return files

    def read_file(self, filename: str) -> bytes:
        return self.local_file(filename).path.read_bytes()

    def read_piece(self, filename: str, piece_id: int, piece_size: int | None = None) -> bytes:
        part = self.piece_path(filename, piece_id)
        if part.exists():
            return part.read_bytes()

        info = self.local_file(filename)
        size = piece_size or info.piece_size
        with info.path.open("rb") as file:
            file.seek(piece_id * size)
            return file.read(size)

    def piece_path(self, filename: str, piece_id: int) -> Path:
        safe_name = filename.replace("/", "_")
        return self.parts_dir / f"{safe_name}.part{piece_id}"

    def has_piece(self, filename: str, piece_id: int) -> bool:
        try:
            info = self.local_file(filename)
            return 0 <= piece_id < info.piece_count
        except FileNotFoundError:
            return self.piece_path(filename, piece_id).exists()

    def write_piece(self, filename: str, piece_id: int, data: bytes) -> Path:
        path = self.piece_path(filename, piece_id)
        path.write_bytes(data)
        return path

    def complete_piece_ids(self, filename: str, piece_count: int) -> list[int]:
        try:
            return list(range(self.local_file(filename).piece_count))
        except FileNotFoundError:
            return [
                piece_id
                for piece_id in range(piece_count)
                if self.piece_path(filename, piece_id).exists()
            ]

    def assemble_file(self, filename: str, piece_count: int) -> Path:
        target = self.download_dir / filename
        with target.open("wb") as output:
            for piece_id in range(piece_count):
                part = self.piece_path(filename, piece_id)
                if not part.exists():
                    raise FileNotFoundError(f"missing piece {piece_id} for {filename}")
                output.write(part.read_bytes())
        return target
