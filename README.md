# RTX 4090 24GB FP32-accumulate notes & diagnostics

[한국어](README.ko.md) | English

Measurement and read-only diagnostics for an **RTX 4090 24GB** that exhibits
**half FP32-accumulate tensor throughput**.

> **This is not a turnkey unlock tool.** It contains no firmware, code, EFI
> binaries or payloads. The tools are read-only and `docs/02` is a high-level
> explanation. **`docs/05` is a reproduction guide for LLMs/agents** that
> explains how to adapt the public upstream implementation to this card's
> profile.

## Observation

Reference card (RTX 4090 24GB, AD102-300):

| Path | Measured | % of peak |
|---|---|---|
| FP16 → FP16 acc | 353.8 TFLOPS | 99.4% |
| FP16 → FP32 acc | 182.9 TFLOPS | 49.6% |
| BF16 → FP32 acc | 182.7 TFLOPS | 49.6% |
| TF32 → FP32 acc | 91.4 TFLOPS | 49.6% |

- Under load: P2, 2805 MHz, 162 W → not power/clock/thermal
- Tensor cores are healthy; **only the FP32-accumulate path is ~50%**
- Details: `docs/01`

## Layout

```text
docs/01-OBSERVATION.ko/en.md    symptom, method, results
docs/02-NOTES.ko/en.md          high-level architecture (no implementation/procedure)
docs/03-DIAGNOSTICS.ko/en.md    probe/benchmark usage
docs/04-SAFETY-LEGAL.ko/en.md   safety, privacy, legal
docs/05-IMPLEMENTATION.ko/en.md * reproduction guide for LLMs/agents
profiles/observed.json          observed card identity / cold state
tools/benchmark/accum.cu        raw MMA (f32acc vs f16acc)
tools/probe/                    AD102-DUMP (read-only capture)
tools/derive_profile.py         capture summary (identity/hashes)
tools/privacy_scan.py           personal/identifier scan
deps/edk2/MdePkg/Include        EDK2 headers (BSD)
```

## Quick start

### 1) Measure throughput

```text
cd tools\benchmark
build.cmd
accum.exe
```

If `f32acc` is half of `f16acc`, the card is in the reduced state (`docs/01`).

### 2) Capture cold state (read-only)

```text
tools\probe\build_probe.cmd
```

Copy `AD102-DUMP.EFI` to `EFI\BOOT\BOOTX64.EFI` on a FAT32 USB and boot it.
Secure Boot OFF, monitor connected to the target GPU. Output:

```text
DUMP00001.LOG / ROM00001.BIN / PROM00001.BIN
```

### 3) Summarize

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

## Requirements

- Windows 11 x64 (the probe is UEFI; Secure Boot OFF)
- Python 3.11+ (summary/scan)
- CUDA Toolkit (benchmark)
- Visual Studio C++ build tools (probe)
- EDK2 headers are vendored

## Safety

- The probe performs no GPU register writes, DMA or FLR.
- The benchmark is a read-only measurement, not a stress test.
- Logs/ROM/PROM are machine-specific; do not commit them.

## Disclaimer

This project is **not affiliated with, endorsed by or sponsored by NVIDIA**.
The code is provided **as is**, and **use is entirely at your own risk**. The
authors and contributors accept no liability for physical damage, data loss,
voided warranties, instability, violation of licenses/contracts/export
controls/local law, or any direct, indirect, special, incidental or
consequential damages.

Anti-circumvention, export-control and EULA rules may apply. This is not legal
advice; obtain qualified legal review before publishing or using this work.

## Implementation guide

`docs/05-IMPLEMENTATION.en.md` explains how to fetch the pinned public upstream
repositories (Zagitta/gpu-unlock, WildFlash1st/cmp90hx-unlock,
allurerr/rtx4090-48gb-unlock) and adapt them to this card's profile
(`profiles/observed.json`). No code, payload or EFI is included here; the guide
points to upstream.

## License

- Original code in `tools/`: **MIT** (`LICENSE`)
- `deps/edk2/MdePkg/Include`: **BSD-2-Clause-Patent** (`LICENSES/`)

Korean: [`README.ko.md`](README.ko.md), `docs/*.ko.md`, `AGENTS.ko.md`.
