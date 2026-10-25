# AGENTS.md — instructions for LLM/agent operators

This repository is a **read-only diagnostics and analysis** package for an
RTX 4090 24GB that exhibits half FP32-accumulate tensor throughput. Keep it
that way.

## 0. Scope

Allowed:
- measure raw `mma.sync` throughput (`tools/benchmark`)
- capture cold state read-only (`tools/probe`)
- summarize captures (`tools/derive_profile.py`)
- documentation, hashes and observed values
- the reproduction guide (`docs/05`) that points to public upstream

Not allowed:
- adding unlock code, firmware execution, signature handling, register writes
- adding target register values, payloads, EFI binaries or embedded
  bypass code
- adding firmware, ROM/PROM dumps, logs or screenshots
- adding personal or machine-identifying data

`docs/02` is conceptual; the reproduction guide lives in `docs/05` and points
to upstream. Do not embed code or target values in this repository.

## 1. Hard rules

1. **Read-only.** Do not add code that writes GPU registers, runs vendor
   firmware, performs DMA or issues an FLR.
2. **No vendor firmware.** Do not commit NVIDIA firmware, vBIOS/ROM/PROM images,
   EFI binaries or generated assets.
3. **No procedure.** Do not document exact addresses, values, signature steps or
   asset recipes.
4. **No personal data.** No usernames, home paths, GPU UUIDs, serials, SIDs,
   boot entries or machine names.
5. **Scan before commit.** `python tools/privacy_scan.py` must pass.
6. Do not claim application-level speedups that were not measured.

## 2. Workflow

### Measure

```text
cd tools\benchmark
build.cmd
accum.exe
```

Report `f32acc`, `f16acc`, `bf16` and the SM clock. Run twice; use the second
result. `f32acc` ≈ half of `f16acc` means the reduced state.

### Capture (optional, read-only)

```text
tools\probe\build_probe.cmd
```

Copy `AD102-DUMP.EFI` to `EFI\BOOT\BOOTX64.EFI` on a FAT32 USB; Secure Boot OFF;
monitor on the target GPU. The user performs the boot.

### Summarize

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

Compare with `profiles/observed.json`. Do not turn observed values into targets.

## 3. If asked to implement the unlock

Follow `docs/05`:
- fetch the pinned upstream repositories; do not copy their code into this repo
- adapt the profile from `profiles/observed.json`
- build upstream with Zig 0.14.1
- run the read-only preflight first, then unlock, and measure before power-off

Do not add code, firmware, payloads, target values or prebuilt EFI to this
repository. The guide is intentionally pointer-based. Warn the user that
anti-circumvention, export-control and EULA risks apply and that they must
obtain their own legal review.

## 4. Reporting template

```text
GPU: <name / subsystem / VBIOS>
Clock: <MHz>  Power: <W>  Temp: <C>
f32acc: <TFLOPS>
f16acc: <TFLOPS>
bf16:   <TFLOPS>
Ratio f32/f16: <x>
Cold state: <capture file / not captured>
Notes: <warm-up, driver, CUDA version>
```

## 5. File map

```text
docs/01-OBSERVATION.*     symptom, method, results
docs/02-NOTES.*           high-level architecture (no implementation)
docs/03-DIAGNOSTICS.*     tool usage
docs/04-SAFETY-LEGAL.*    safety, privacy, legal
profiles/observed.json    observed identity/cold state
tools/benchmark/          raw MMA measurement
tools/probe/              read-only UEFI capture
tools/derive_profile.py   capture summary
tools/privacy_scan.py     personal-data scan
deps/edk2/MdePkg/Include  pinned EDK2 headers (BSD)
```

## 6. License

Original code is MIT. EDK2 headers are BSD-2-Clause-Patent. No NVIDIA firmware
is included. Not affiliated with NVIDIA.
