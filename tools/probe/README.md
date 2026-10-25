# AD102-DUMP — read-only cold-state capture

A UEFI application that reads the cold-state registers, PCI config, ROM and
PROM of one NVIDIA GPU and writes a log plus binary dumps to the USB root.
It is a diagnostic tool: it does not write GPU registers, does not run
firmware, does not use DMA and does not reset the device.

## What it captures

- 38 registers used to identify the chip, VRAM size and observed cold state
  (speed-selector and permission values, WPR/WPR2, FUSEC, SEC2 status)
- 16 FWSEC fuse words
- PCI config 0x00–0x1FF
- the PCI ROM image and a 0x9A200-byte PROM view at BAR0+0x300000

Output on the volume it was loaded from:

```text
DUMPxxxxx.LOG     registers, PCI config, sizes
ROMxxxxx.BIN      PCI ROM image
PROMxxxxx.BIN     PROM view
```

Hashes are **not** computed in the probe. `tools/derive_profile.py` hashes the
two `.BIN` files on the host.

## What it does not do

- No GPU MMIO writes, no DMA, no FLR, no driver ownership changes.
- The only possible PCI config change is enabling the memory-decode bit when it
  is off and BAR0 is already valid (the same thing an OS does at boot). If BAR0
  is unassigned, it writes nothing and skips MMIO.

## Build

Windows with Visual Studio C++ build tools:

```text
tools\probe\build_probe.cmd
```

The script uses the EDK2 headers in `deps/edk2/MdePkg/Include` and produces
`tools\probe\AD102-DUMP.EFI`.

## Run

1. Format a small USB as FAT32.
2. Copy `AD102-DUMP.EFI` to `EFI\BOOT\BOOTX64.EFI`.
3. Disable Secure Boot, connect the monitor to the target GPU, boot the USB.
4. Press a key when it finishes.
5. Copy the three output files back.

## Summarize

```text
python tools/derive_profile.py DUMP00001.LOG
python tools/derive_profile.py DUMP00001.LOG --json > observed.json
```

The summary prints the device identity, the observed cold state and SHA-256
hashes of the ROM/PROM dumps. It contains no target values or procedure.

## Privacy

The log and dumps are machine-specific. Do **not** commit them. They include
the BAR address, PCI config and the card's ROM/PROM contents.
