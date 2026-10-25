# SPDX-License-Identifier: MIT
"""Summarize a read-only AD102-DUMP capture.

The probe writes DUMPxxxxx.LOG, ROMxxxxx.BIN and PROMxxxxx.BIN. This script
extracts the device identity, the observed cold state and SHA-256 hashes of
the dumped ROM/PROM images. It is diagnostic only: it does not contain or
derive any target register values or unlock procedure.

Usage:
    python tools/derive_profile.py DUMP00001.LOG
    python tools/derive_profile.py DUMP00001.LOG --json > observed.json
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import sys

REG = re.compile(r"^REG (0x[0-9a-fA-F]+) (0x[0-9a-fA-F]+)")
CFG = re.compile(r"^CFG (0x[0-9a-fA-F]+) (0x[0-9a-fA-F]+)")
DEV = re.compile(r"^DEV .*id=(0x[0-9a-fA-F]+)")
ROM = re.compile(r"^ROM size=(\d+)")
PROM = re.compile(r"^PROM size=(\d+)")


def parse(path: Path) -> dict:
    out = {"regs": {}, "cfg": {}, "rom_size": None, "prom_size": None, "id": None}
    for line in path.read_text(errors="replace").splitlines():
        m = REG.match(line)
        if m:
            out["regs"][int(m.group(1), 16)] = int(m.group(2), 16)
            continue
        m = CFG.match(line)
        if m:
            out["cfg"][int(m.group(1), 16)] = int(m.group(2), 16)
            continue
        m = DEV.match(line)
        if m:
            out["id"] = int(m.group(1), 16)
            continue
        m = ROM.match(line)
        if m:
            out["rom_size"] = int(m.group(1))
            continue
        m = PROM.match(line)
        if m:
            out["prom_size"] = int(m.group(1))
    return out


def seq_of(log: Path) -> str:
    m = re.search(r"(\d{5})", log.name)
    return m.group(1) if m else ""


def find_bin(log: Path, prefix: str) -> Path | None:
    seq = seq_of(log)
    exact = log.parent / f"{prefix}{seq}.BIN"
    if exact.is_file():
        return exact
    cands = sorted(log.parent.glob(f"{prefix}*.BIN"))
    return cands[0] if cands else None


def sha_or_none(path: Path | None) -> str | None:
    if not path or not path.is_file():
        return None
    return hashlib.sha256(path.read_bytes()).hexdigest()


def reg(d: dict, off: int) -> int:
    return d["regs"].get(off, 0)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("log", type=Path)
    ap.add_argument("--rom", type=Path)
    ap.add_argument("--prom", type=Path)
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()

    d = parse(a.log)
    rom = a.rom or find_bin(a.log, "ROM")
    prom = a.prom or find_bin(a.log, "PROM")
    device = d["cfg"].get(0x00, d["id"] or 0)
    subsystem = d["cfg"].get(0x2C, 0)

    observed = {
        "schema_version": 1,
        "source_log": a.log.name,
        "gpu": {
            "device_id": f"{device & 0xffff:04x}:{(device >> 16) & 0xffff:04x}",
            "subsystem_id": f"{subsystem & 0xffff:04x}:{(subsystem >> 16) & 0xffff:04x}",
            "chip_register": f"{reg(d, 0x00000000):#x}",
            "vram_mib": reg(d, 0x001183A4),
        },
        "cold_state": {
            "ss0": f"{reg(d, 0x0082381C):#x}",
            "ss1": f"{reg(d, 0x00823820):#x}",
            "feat_plm": f"{reg(d, 0x00823804):#x}",
            "plm": f"{reg(d, 0x001FA7C8):#x}",
            "wpr2_hi": f"{reg(d, 0x001FA828):#x}",
            "source_raw": f"{reg(d, 0x000823B00):#x}",
        },
        "firmware": {
            "pci_rom_bytes": d["rom_size"],
            "pci_rom_sha256": sha_or_none(rom),
            "prom_bytes": d["prom_size"],
            "prom_sha256": sha_or_none(prom),
        },
        "note": "Observed values only. This file contains no target values or procedure.",
    }

    if a.json:
        print(json.dumps(observed, indent=2))
        return 0

    print(f"# source: {a.log}")
    print(f"device_id      = {observed['gpu']['device_id']}")
    print(f"subsystem_id   = {observed['gpu']['subsystem_id']}")
    print(f"chip           = {observed['gpu']['chip_register']}")
    print(f"vram_mib       = {observed['gpu']['vram_mib']}")
    print(f"ss0_cold       = {observed['cold_state']['ss0']}")
    print(f"ss1_cold       = {observed['cold_state']['ss1']}")
    print(f"feat_plm       = {observed['cold_state']['feat_plm']}")
    print(f"plm            = {observed['cold_state']['plm']}")
    print(f"wpr2_hi        = {observed['cold_state']['wpr2_hi']}")
    print(f"source_raw     = {observed['cold_state']['source_raw']}  (diagnostic only)")
    print(f"pci_rom_bytes  = {observed['firmware']['pci_rom_bytes']}")
    print(f"pci_rom_sha256 = {observed['firmware']['pci_rom_sha256']}")
    print(f"prom_bytes     = {observed['firmware']['prom_bytes']}")
    print(f"prom_sha256    = {observed['firmware']['prom_sha256']}")
    if not rom or not prom:
        print("\nWARNING: ROM/PROM binary not found next to the log; hashes are null.", file=sys.stderr)
        return 2
    print("\nObserved values only. Compare with profiles/observed.json; this tool does not")
    print("contain or derive any target register value or unlock procedure.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
