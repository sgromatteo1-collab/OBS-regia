#!/usr/bin/env python3
"""Restore the exact Pallanuoto Native 0.4.3 source archive from repository parts."""
from pathlib import Path, PurePosixPath
import base64
import hashlib
import io
import tarfile

parts = sorted(Path("payload").glob("part*.txt"))
assert [p.name for p in parts] == [f"part{i:02}.txt" for i in range(7)], "One or more archive parts are missing"
encoded = "".join(p.read_text(encoding="ascii").strip() for p in parts)
raw = base64.b64decode(encoded, validate=True)
digest = hashlib.sha256(raw).hexdigest()
expected = "ed77913c1e2681f479e86b5404d2ddd8e325935f498f382200277ed30e78014b"
if digest != expected:
    raise SystemExit(f"Archive checksum mismatch: {digest} != {expected}")
with tarfile.open(fileobj=io.BytesIO(raw), mode="r:xz") as archive:
    for member in archive.getmembers():
        name = PurePosixPath(member.name)
        if name.is_absolute() or ".." in name.parts or not member.isfile():
            raise SystemExit(f"Unsafe archive entry: {member.name}")
    archive.extractall(".")
print(f"RESTORED source archive: {len(raw)} bytes; SHA-256={digest}")
