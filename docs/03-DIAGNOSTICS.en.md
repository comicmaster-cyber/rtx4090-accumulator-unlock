# 03 — Using the diagnostics

## 1. Raw MMA measurement

```text
cd tools\benchmark
build.cmd
accum.exe
```

- Requires the CUDA Toolkit; builds for `sm_89`
- Warms up, then measures 5 times
- Run it twice and use the second result (clock ramp)

Interpretation:

| Observation | Meaning |
|---|---|
| `f32acc` ≈ `f16acc` | normal (restored) state |
| `f32acc` ≈ half of `f16acc` | reduced state described in `docs/01` |
| both low | clock ramp, power or thermal issue |

~200 W is normal: a tensor-only loop does not use the whole TGP.

## 2. Cold-state capture (read-only)

```text
tools\probe\build_probe.cmd
```

1. Copy `AD102-DUMP.EFI` to `EFI\BOOT\BOOTX64.EFI` on a FAT32 USB
2. Secure Boot OFF, monitor connected to the target GPU
3. Boot the USB; press a key when it finishes
4. Collect the output from the USB root:

```text
DUMP00001.LOG
ROM00001.BIN
PROM00001.BIN
```

The probe performs no GPU register writes, no DMA, no FLR and no ownership
changes. If BAR0 is unassigned it writes nothing. The only possible config
change is enabling the PCI memory-decode bit when it is off and BAR0 is valid.

## 3. Summarize a capture

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

Output:
- device/subsystem/chip/VRAM
- observed cold state (selectors/permissions/WPR)
- ROM/PROM sizes and SHA-256

Compare with `profiles/observed.json` to see whether the card is the same
family. The tool does **not** print target values or a procedure.

## 4. Privacy and distribution

- `DUMP*.LOG`, `ROM*.BIN`, `PROM*.BIN` are **machine-specific**.
- They contain the BAR address, PCI config and the card's ROM/PROM contents.
- Do not commit them (they are in `.gitignore`).
- Before sharing, remove identifiers or keep only hashes.

## 5. Troubleshooting

| Symptom | Check |
|---|---|
| Probe does not start | Secure Boot OFF, FAT32, `EFI\BOOT\BOOTX64.EFI` path |
| `REGS skipped: no valid BAR0` | GPU BAR assignment / Above 4G in BIOS |
| `PROM read-error` | BAR0 size/window |
| Empty log | USB writability, volume root |
| Low first benchmark result | warm up for 10 s and rerun |
