# Raw MMA measurement

`accum.cu` measures `mma.sync` throughput for three accumulator types on the
same loop structure:

- `f32acc` — FP16 inputs, **FP32 accumulate**
- `f16acc` — FP16 inputs, FP16 accumulate
- `bf16`   — BF16 inputs, FP32 accumulate

It is a read-only hardware measurement: it allocates device memory, runs MMA
loops, and copies results back. It does not write registers, firmware or
settings.

## Build and run

Windows (CUDA toolkit on PATH):

```text
build.cmd
accum.exe
```

Linux:

```text
nvcc -O3 -std=c++17 -arch=sm_89 accum.cu -o accum
./accum
```

Run it twice. The binary warms up, but a GPU that was idle can still report a
lower first result while the boost clock ramps.

## Reference observation

On the reference RTX 4090 24GB (450 W, warm):

```text
f32acc  ~183 TFLOPS
f16acc  ~354 TFLOPS
bf16    ~183 TFLOPS
```

After a firmware-level state change performed outside this repository, the
same card measured:

```text
f32acc  ~335-366 TFLOPS
f16acc  ~354-359 TFLOPS
bf16    ~354 TFLOPS
```

The interesting signal is the **ratio**: if `f32acc` is about half of
`f16acc`, the card is in the reduced state described in `docs/01`.

## Notes

- This is not an application benchmark. It does not use cuBLAS, so it is
  independent of kernel selection and driver heuristics.
- Power draw under a tensor-only loop is much lower than the board TGP. That is
  expected and not evidence of throttling.
- Performance depends on the boost clock. Report the clock alongside the number
  if you compare runs.
