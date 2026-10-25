# 05 — Implementation guide for LLMs/agents

> This is a **reproduction guide**. The repository contains no code, payloads,
> EFI binaries or firmware. It explains how to take the public upstream
> implementation and adapt it to **this card's profile**.
> The legal risk is high. Read `docs/04-SAFETY-LEGAL.en.md` first.

## 0. Prerequisites

- Target card: identifiers and cold state in `profiles/observed.json`
- Cold boot, **Secure Boot OFF**, **Fast Startup OFF**
- Monitor connected to the target GPU
- Exactly one `bootmgfw.efi` on the Windows ESP
- Python 3.11+, **Zig 0.14.1**, (probe only) MSVC
- One USB stick (fully erased)

## 1. Upstream (public)

| Repository | Pinned commit | Role |
|---|---|---|
| `Zagitta/gpu-unlock` | `0ebdb1c6308b6a2072be5bf04513472942541956` | original research core (GPL-2.0) |
| `WildFlash1st/cmp90hx-unlock` | `76940ae583281daa2b883518a2e89b56c11d3a46` | V67 signature reference (rejoin15 0014) |
| `allurerr/rtx4090-48gb-unlock` | `fd8fcb14dc5a6ed5ce9eea40e4cbb453c800df80` | UEFI/AD102 port and asset generators |

```text
git clone https://github.com/allurerr/rtx4090-48gb-unlock.git
cd rtx4090-48gb-unlock
git checkout fd8fcb14dc5a6ed5ce9eea40e4cbb453c800df80
```

## 2. Adapt the profile

The upstream targets a **48GB card**. Change the following for this 24GB card.
Take the values from `profiles/observed.json`.

| File | Fields |
|---|---|
| `src/unlock/port.h` | device, subsystem, chip, VRAM, SS0/SS1 cold |
| `src/unlock/prom_profile.h` | PROM view size/hash |
| `src/unlock/app.c` | PCI ROM size/hash, accepted VRAM, selector pair |
| `profiles/card.json` | keep in sync with the values above |

**Do not modify the core logic (SEC2/FWSEC/Booter/V67/FLR).**
Keep the upstream selector targets and signature handling unchanged.

## 3. Generate assets

```text
python -m pip install --only-binary=:all: --target local/python-deps zstandard==0.25.0
python tools/setup_firmware.py --download
```

- NVIDIA kernel-module-source **610.43.03**
- CachyOS nvidia-utils **610.43.03**
- V67: `tools/firmware/ad102_payload.py`
- The six resulting assets must match the hashes in `profiles/assets.json`.

## 4. Build

```text
python tools/build.py --zig zig
```

- Zig **0.14.1** required
- Outputs `payload/PREFLIGHT.EFI` and `payload/UNLOCK.EFI`
- Verify PE/AMD64/EFI application, no imports, relocations present

## 5. Boot media

- GPT + **exactly 512 MiB (1,048,576 sectors)** FAT32
- Layout:

```text
EFI\BOOT\BOOTX64.EFI                 PREFLIGHT (first boot)
payload\PREFLIGHT.EFI
payload\UNLOCK.EFI
RTX4090-48GB-AUTO\ASSETS\            five assets
RTX4090-48GB-AUTO\STATE.BIN          96 B, "4090AUT1", byte[8]=0
```

## 6. Run order

1. **Preflight** (read-only): boot and inspect the log
   - chip, cold state, ROM/PROM hashes, assets, Windows preload
   - `PREFLIGHT_ONLY PASS`, `FINAL status=0 ... clean=1`
2. **Unlock**: copy `UNLOCK.EFI` to `EFI\BOOT\BOOTX64.EFI`, set `STATE.BIN[8]=0`
3. Boot → wait 3 s (or press `U`) → black screen → FLR → Windows
4. **Measure before powering off** (`tools/benchmark`)

## 7. Verification

```text
transaction result=0
FLR=1
SELECTOR ... matched=1
POST-GOP ... expected SS0/SS1
FINAL status=0 ... clean=1
f32acc ≈ f16acc
```

- `STOP: cleanup cannot be proven` → **physically power off**; do not force Windows
- Do not retry a failed transaction in a loop

## 8. LLM checklist

- [ ] fetched upstream at the **pinned commits**
- [ ] updated the profile from `profiles/observed.json`
- [ ] did not modify the core logic
- [ ] all six asset hashes match
- [ ] built with Zig 0.14.1
- [ ] passed the read-only preflight first
- [ ] measured before powering off
- [ ] did not commit firmware/ROM/PROM/logs
- [ ] `python tools/privacy_scan.py` passes

## 9. Legal and safety

- This guide carries **high legal risk** (anti-circumvention, export control, EULA).
- The repository distributes no code or firmware, but a reproduction guide can
  still be regulated.
- Obtain qualified legal review before use or publication. This is not legal advice.
