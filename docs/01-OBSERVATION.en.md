# 01 — Observation: only the FP32-accumulate path is halved

## 1. Symptom

Measured `mma.sync` throughput by accumulator type on the reference card
(RTX 4090 24GB, AD102-300):

| Path | Measured | % of theoretical peak |
|---|---|---|
| FP16 → FP16 acc | 353.8 TFLOPS | 99.4% |
| FP16 → FP32 acc | 182.9 TFLOPS | 49.6% |
| BF16 → FP32 acc | 182.7 TFLOPS | 49.6% |
| TF32 → FP32 acc | 91.4 TFLOPS | 49.6% |

- Under load: **P2, 2805 MHz, 162 W**
- `clock64()` measured 2686 MHz SM clock
- Non-tensor FP32 throughput was normal

The tensor cores are healthy; **only the FP32-accumulate path runs at about
50% of peak**. PyTorch/cuBLAS use FP32 accumulation by default, so real
workloads lose roughly 2x.

## 2. Method

- `tools/benchmark/accum.cu` — raw `mma.sync` loop, 128 SMs, 1024 blocks × 256 threads
- `f32acc` / `f16acc` / `bf16` on the same loop structure
- Warm-up, then 5 measurements, take the best
- cuBLAS `cublasGemmEx` at n=4096 as a secondary check

## 3. Detailed results

### Raw MMA

| Path | Before | After state change | Ratio |
|---|---|---|---|
| FP16 → FP32 acc (chains=8) | 182.9 | 353.7 | 1.93× |
| FP16 → FP32 acc (chains=1) | 182.3 | 333.7 | 1.83× |
| BF16 → FP32 acc (chains=8) | 182.7 | 354.4 | 1.94× |
| TF32 → FP32 acc (chains=8) | 91.4 | 176.9 | 1.94× |
| FP16 → FP16 acc (chains=8) | 353.8 | 358.6 | 1.01× |

### cuBLAS GEMM (n=4096)

| Mode | Before | After state change | Change |
|---|---|---|---|
| FP32 pedantic (non-tensor) | 54.8 | 54.9 | 0% |
| FP16 / FP32 acc | 162.9 | 216.1 | +33% |
| BF16 / FP32 acc | 154.0 | 156.1 | +1% |
| TF32 / FP32 acc | 88.8 | 150.5 | +70% |

n=8192 (after state change, reference): FP32 55.6 / FP16 183.0 / BF16 189.8 / TF32 146.1.

### Stability

- 31 s sustained FP32-accumulate load: min 330.2, median 333.3 TFLOPS
- 60 °C, ~215 W, zero WHEA errors

## 4. Interpretation

- This is not a power, clock or thermal problem. At peak clock, one path reaches
  99% of peak while another reaches only half.
- ~200 W is normal: a tensor-only loop does not use the 450 W TGP.
- cuBLAS numbers vary with shape, kernel and clock; raw MMA is the hardware check.
- Application-level speedups (ComfyUI, PyTorch, etc.) are workload-dependent and
  not guaranteed.

## 5. Limits

- One reference card, one VBIOS, one driver combination
- Long-duration stability, sleep/hibernate and all applications were not tested
- Cause analysis is in `docs/02` (high level; no implementation or procedure)
