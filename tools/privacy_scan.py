# SPDX-License-Identifier: GPL-2.0-only
"""Fail closed if this repository contains personal or machine-identifying data.

Run before every commit and in CI:
    python tools/privacy_scan.py
Exit code 0 = clean, 1 = findings.
"""
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

SKIP_DIRS = {".git", "deps", "local", "build", "payload", "__pycache__", "dist"}
SKIP_FILES = {"tools/privacy_scan.py"}
ALLOWED_SUFFIXES = {
    "", ".md", ".py", ".ps1", ".cmd", ".c", ".h", ".inc", ".json", ".yml",
    ".yaml", ".txt", ".cu", ".gitignore", ".gitattributes",
}
FORBIDDEN_SUFFIXES = {
    ".efi", ".bin", ".rom", ".log", ".png", ".jpg", ".jpeg", ".gif", ".zip",
    ".7z", ".obj", ".pdb", ".sys", ".dll", ".exe", ".state", ".vhd", ".vhdm",
}

PATTERNS = [
    ("windows-user-path", re.compile(r"C:\\\\Users\\\\", re.I)),
    ("unix-home-path", re.compile(r"/home/[A-Za-z0-9._-]+")),
    ("drive-absolute-path", re.compile(r"(?<![A-Za-z0-9])[A-Za-z]:\\\\")),
    ("gpu-uuid", re.compile(r"GPU-[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}", re.I)),
    ("windows-sid", re.compile(r"S-1-5-21(?:-\d+){3,}")),
    ("boot-entry", re.compile(r"\bBOOT[0-9A-F]{4}\b")),
    ("mac-address", re.compile(r"\b(?:[0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}\b")),
    ("ipv4", re.compile(r"\b(?:\d{1,3}\.){3}\d{1,3}\b")),
    ("email", re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}")),
    ("env-identity", re.compile(r"(?i)\b(computername|userprofile|username)\b")),
]


def iter_files():
    # Prefer the tracked file list so generated/gitignored output never fails
    # the scan. Fall back to the working tree before the first commit.
    try:
        out = subprocess.check_output(
            ["git", "-C", str(ROOT), "ls-files", "-z"],
            stderr=subprocess.DEVNULL,
        )
        names = [n for n in out.decode("utf-8", "replace").split("\0") if n]
        if names:
            for name in names:
                rel = Path(name)
                if any(part in SKIP_DIRS for part in rel.parts):
                    continue
                p = ROOT / name
                if p.is_file() and name not in SKIP_FILES:
                    yield p, rel
            return
    except Exception:
        pass
    for p in sorted(ROOT.rglob("*")):
        if not p.is_file():
            continue
        rel = p.relative_to(ROOT)
        if any(part in SKIP_DIRS for part in rel.parts):
            continue
        if rel.as_posix() in SKIP_FILES:
            continue
        yield p, rel


def main() -> int:
    findings = []
    checked = 0
    for path, rel in iter_files():
        suffix = path.suffix.lower()
        if suffix in FORBIDDEN_SUFFIXES:
            findings.append(f"{rel}: forbidden file type ({suffix})")
            continue
        if suffix not in ALLOWED_SUFFIXES:
            findings.append(f"{rel}: unexpected file type ({suffix or 'none'})")
            continue
        # Third-party license texts are verbatim and may contain vendor
        # contact addresses; they are not personal data.
        if rel.parts and rel.parts[0] == "LICENSES":
            continue
        if path.stat().st_size > 2_000_000:
            findings.append(f"{rel}: unexpectedly large text file")
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="strict")
        except UnicodeDecodeError:
            findings.append(f"{rel}: not UTF-8 text (binary?)")
            continue
        checked += 1
        for name, rx in PATTERNS:
            for m in rx.finditer(text):
                line = text.count("\n", 0, m.start()) + 1
                findings.append(f"{rel}:{line}: {name}: {m.group(0)[:60]}")

    print(f"scanned {checked} text files")
    if findings:
        print(f"FAIL: {len(findings)} finding(s)")
        for f in findings[:200]:
            print(" -", f)
        return 1
    print("PASS: no personal or machine-identifying patterns found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
