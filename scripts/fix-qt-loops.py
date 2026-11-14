#!/usr/bin/env python3
"""Resolve Qt6 QJsonArray range-loop warnings treated as errors by Xcode."""
from pathlib import Path

source = Path("src/plugin.cpp")
data = source.read_text(encoding="utf-8")
changes = [
    ("for(const auto &p:players){", "for(const auto p:players){"),
    ('for(const auto &v:t["players"].toArray()){', 'for(const auto v:t["players"].toArray()){'),
]
for old, new in changes:
    if data.count(old) != 1:
        raise SystemExit(f"Expected exactly one occurrence for: {old}")
    data = data.replace(old, new, 1)
source.write_text(data, encoding="utf-8")
print("Corrected 2 Qt6 range-loop references for macOS Clang.")
