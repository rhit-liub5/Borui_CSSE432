"""Local file management for peers."""

from __future__ import annotations

import shutil
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class LocalFile:
    filename: str
    path: Path
    filesize: int


class FileManager:
    def __init__(self, shared_dir: str, download_dir: str):
        self.shared_dir = Path(shared_dir)
        self.download_dir = Path(download_dir)
        self.shared_dir.mkdir(parents=True, exist_ok=True)
        self.download_dir.mkdir(parents=True, exist_ok=True)

    def local_file(self, filename: str) -> LocalFile:
        path = self.shared_dir / filename
        if not path.exists():
            path = self.download_dir / filename
        if not path.exists():
            raise FileNotFoundError(filename)
        filesize = path.stat().st_size
        return LocalFile(filename, path, filesize)

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

    def write_file(self, filename: str, data: bytes) -> Path:
        target = self.download_dir / filename
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        return target
