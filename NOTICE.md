# NOTICE

This repository contains **read-only diagnostics and analysis** for an
RTX 4090 24GB that exhibits reduced FP32-accumulate tensor throughput. It does
not contain unlock code, firmware, prebuilt EFI binaries, payloads, register
targets or a step-by-step bypass procedure.

## Code license

- Original code in `tools/` is licensed under the **MIT License** (see
  `LICENSE`).
- `deps/edk2/MdePkg/Include` is from TianoCore EDK2, commit
  `c1415771ac592ad8509351c46cd3cda4f38d7d52`, under
  **BSD-2-Clause-Patent** (see `LICENSES/EDK2-BSD-2-Clause-Patent.txt`).
  The EDK2 headers are required only to build the read-only UEFI probe.

## No vendor firmware

No NVIDIA firmware, vBIOS image, ROM/PROM dump, EFI binary, log or capture is
included. `profiles/observed.json` contains only observed identifiers and
hashes of images read back from a card, for comparison.

## No affiliation

This project is not affiliated with, endorsed by or sponsored by NVIDIA.
"NVIDIA", "GeForce" and "RTX" are trademarks of their respective owners and
are used descriptively only.

## Legal note

The documentation intentionally omits register addresses, target values,
signature handling and asset-generation recipes. Nothing here is legal advice.
Anti-circumvention, export-control and contract rules may apply in your
jurisdiction; consult a qualified lawyer before publishing or using this work.
