# 02 — Architecture notes (high level)

> This document is a **conceptual explanation**. It contains no implementation
> code, target values, signature handling or asset recipes. The reproduction
> procedure is in `docs/05-IMPLEMENTATION.en.md`, which points to the public
> upstream. Reproduction is the reader's responsibility, and the legal risk
> must be reviewed separately.

## 1. What is known

- The FP16-accumulate path reaches 99% of peak.
- Only the FP32-accumulate path runs at about 50% of peak.
- It is not a power, clock, thermal or driver-kernel-selection problem.
- The state is established early in boot, **before the OS driver binds**.
- A power cycle resets it.

## 2. Conceptual model

At boot the GPU latches a **speed-selector** and **permission-level** state that
the driver later consumes. On the observed card, that initial state limits the
FP32-accumulate path to half rate.

Conceptually, changing it requires:

1. A UEFI component that runs **before the driver loads**.
2. Cooperation with the vendor's boot firmware chain (loading and verification).
3. Overriding the state, then performing **exactly one device reset (FLR)** so the
   driver starts from a clean state.
4. Restoring GOP/display and handing off to Windows.

## 3. Why it is hard

- The firmware authentication/signature chain and permission levels must be understood.
- WPR (Wipe Protected Region) memory layout and boot ordering must line up.
- Wrong reset/handoff ordering can leave a black screen or an unstable device.
- Recovery from a failed attempt is a **physical power-off**.

## 4. Verification criteria (without values)

Success requires all of:

- re-reading the selector state after the change and confirming the intended state
- **exactly one FLR**
- DMA/quiescence cleanup and bus-mastering cleared
- GOP/display restored and Windows boots normally
- `tools/benchmark` shows `f32acc` close to `f16acc`

Do not claim success from a log line or a recovered screen alone. Measure.

## 5. Volatility

- The state is re-established at every power-on.
- It must be re-applied after a reboot or power loss.
- It is not a permanent patch and is not a vBIOS flash.

## 6. Scope of this repository

Included:
- observation and measurement (`docs/01`, `tools/benchmark`)
- read-only cold-state capture (`tools/probe`)
- capture summary (`tools/derive_profile.py`)
- card identity/hashes (`profiles/observed.json`)

Intentionally excluded:
- firmware execution/authentication code
- selector/permission write code and target values
- asset generators, EFI binaries and boot-media builders (the reproduction
  guide is in `docs/05`)

## 7. Legal note

- Not affiliated with, endorsed by or sponsored by NVIDIA.
- Anti-circumvention, export-control and contract (EULA) rules may apply.
- This is not legal advice. Obtain qualified legal review before publishing or
  using this work.
