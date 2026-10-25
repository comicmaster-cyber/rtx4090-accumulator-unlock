# 04 — Safety, privacy and legal notice

## 1. Safety

The tools in this repository are **read-only diagnostics**.

- `tools/probe`: no GPU register writes, no DMA, no FLR, no ownership changes.
  If BAR0 is unassigned it writes nothing. The only possible config change is
  enabling the PCI memory-decode bit when it is off and BAR0 is valid (what an
  OS does at boot).
- `tools/benchmark`: allocates device memory and runs MMA loops. It does not
  touch registers or firmware. It is not a stress test; watch temperature and
  power.
- `tools/derive_profile.py`: reads files and computes hashes.

This repository contains **no unlock code, firmware, EFI binaries, asset
generators, target register values or procedure.** `docs/02` is conceptual only.

## 2. Privacy

- `DUMP*.LOG`, `ROM*.BIN`, `PROM*.BIN` are machine-specific. They contain the
  BAR address, PCI config and the card's ROM/PROM contents.
- Do not commit them (they are in `.gitignore`).
- Before sharing, remove identifiers or keep only hashes.
- `tools/privacy_scan.py` checks for path and identifier patterns.

## 3. Legal notice

- This project is **not affiliated with, endorsed by or sponsored by NVIDIA**.
- "NVIDIA", "GeForce" and "RTX" are trademarks of their respective owners and
  are used descriptively only.
- The code is provided **as is**, without warranty of merchantability, fitness
  for a particular purpose or non-infringement.
- **Use is entirely at your own risk.** The authors and contributors accept no
  liability for physical damage, data loss, voided warranties, instability,
  reduced performance, violation of licenses, contracts, export controls or
  local law, or any direct, indirect, special, incidental or consequential
  damages.
- **Anti-circumvention**, **export-control** and **EULA (reverse engineering /
  modification)** rules may apply. This is not legal advice. Obtain qualified
  legal review before publishing or using this work.
- If the observed limitation is related to **export-compliance features**, then
  publishing or distributing a bypass may carry additional legal risk.

## 4. Limits of the data and claims

- Measurements come from one reference card, one VBIOS and one driver.
- cuBLAS numbers vary with shape, kernel and clock.
- Application-level speedups (ComfyUI, PyTorch, etc.) are workload-dependent and
  not guaranteed.
- Long-duration stability, sleep/hibernate and all applications were not tested.

## 5. Responsible publication

- Do not commit firmware, ROM/PROM dumps, EFI binaries or logs.
- Remove personal and machine-identifying data.
- Review the legal risk before publishing.
- Do not extend this repository beyond its diagnostics/documentation scope.
