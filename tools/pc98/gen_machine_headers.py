#!/usr/bin/env python3
"""Generate deterministic PC-98 forwarding headers from a reviewed name list."""

from __future__ import annotations

import argparse
from pathlib import Path


BODY_HEADERS = {
    "_bus.h",
    "bus.h",
    "efi.h",
    "md_var.h",
    "param.h",
    "ppireg.h",
    "timerreg.h",
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", type=Path, default=Path.cwd())
    parser.add_argument("--names", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()

    src = args.src.resolve()
    output = src / "sys/pc98/include"
    output.mkdir(parents=True, exist_ok=True)
    names = []
    for raw in args.names.read_text(encoding="utf-8").splitlines():
        name = raw.split("#", 1)[0].strip()
        if name:
            names.append(name)
    if names != sorted(set(names)):
        raise SystemExit("header list must be sorted and unique")

    rows = ["header\ttarget\toperation"]
    for name in names:
        if name in BODY_HEADERS:
            raise SystemExit(f"body header must not be generated: {name}")
        candidates = [
            (src / "sys/i386/include" / name, f"i386/{name}"),
            (src / "sys/x86/include" / name, f"x86/{name}"),
        ]
        target = next((inc for path, inc in candidates if path.is_file()), None)
        if target is None:
            raise SystemExit(f"no stable destination header for {name}")
        path = output / name
        if path.exists():
            raise SystemExit(f"refusing to overwrite existing header: {path}")
        path.write_text(f"#include <{target}>\n", encoding="ascii")
        rows.append(f"{name}\t{target}\tgenerated-forwarder")

    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text("\n".join(rows) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
