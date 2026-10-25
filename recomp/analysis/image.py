"""Read-only view of a PE32 image (or a synthetic one for tests) at its virtual addresses."""
from __future__ import annotations

import hashlib
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
VAGANTE_SHA256 = "9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c"  # Steam 1.10.1
SECTION_EXEC = 0x20000000


@dataclass(frozen=True)
class Section:
    name: str
    start: int
    data: bytes  # padded with zeros to the virtual size
    executable: bool

    @property
    def end(self) -> int:
        return self.start + len(self.data)


class Image:
    def __init__(self, base: int, entry: int, sections: list[Section],
                 imports: dict[int, tuple[str, str]] | None = None, sha256: str = ""):
        self.base = base
        self.entry = entry
        self.sections = sorted(sections, key=lambda s: s.start)
        self.imports = imports or {}
        self.sha256 = sha256

    @classmethod
    def from_pe(cls, path: Path, expected_sha256: str | None = VAGANTE_SHA256) -> "Image":
        sys.path.insert(0, str(ROOT / "local/python-deps"))
        import pefile

        raw = path.read_bytes()
        digest = hashlib.sha256(raw).hexdigest()
        if expected_sha256 and digest != expected_sha256:
            raise SystemExit(f"reference SHA256 mismatch: {digest}")
        pe = pefile.PE(data=raw)
        base = pe.OPTIONAL_HEADER.ImageBase
        sections = []
        for s in pe.sections:
            size = max(s.Misc_VirtualSize, s.SizeOfRawData)
            data = s.get_data()[:size].ljust(s.Misc_VirtualSize, b"\0")
            sections.append(Section(s.Name.rstrip(b"\0").decode("ascii", "replace"),
                                    base + s.VirtualAddress, data[:s.Misc_VirtualSize],
                                    bool(s.Characteristics & SECTION_EXEC)))
        imports = {}
        for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
            dll = entry.dll.decode("ascii", "replace")
            for imp in entry.imports:
                name = imp.name.decode("ascii", "replace") if imp.name else f"#ord{imp.ordinal}"
                imports[imp.address] = (dll, name)
        return cls(base, base + pe.OPTIONAL_HEADER.AddressOfEntryPoint, sections, imports, digest)

    def section_at(self, va: int) -> Section | None:
        for s in self.sections:
            if s.start <= va < s.end:
                return s
        return None

    def is_exec(self, va: int) -> bool:
        s = self.section_at(va)
        return bool(s and s.executable)

    def read(self, va: int, size: int) -> bytes:
        s = self.section_at(va)
        if not s:
            return b""
        offset = va - s.start
        return s.data[offset:offset + size]

    def u8(self, va: int) -> int | None:
        b = self.read(va, 1)
        return b[0] if b else None

    def u32(self, va: int) -> int | None:
        b = self.read(va, 4)
        return struct.unpack("<I", b)[0] if len(b) == 4 else None

    @property
    def exec_sections(self) -> list[Section]:
        return [s for s in self.sections if s.executable]

    @property
    def data_sections(self) -> list[Section]:
        return [s for s in self.sections if not s.executable]
