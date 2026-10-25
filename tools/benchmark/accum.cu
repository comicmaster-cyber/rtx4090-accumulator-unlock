// Compare raw mma.sync throughput by accumulator type.
// Build: nvcc -O3 -std=c++17 -arch=sm_89 accum.cu -o accum
// The decisive number is f32acc vs f16acc. In the reduced state f32acc is
// about half of f16acc. This binary is read-only measurement; it does not
// change any GPU state.
#include <cstdio>
#include <cuda_runtime.h>

__global__ void k_f32acc(unsigned* out, int iters, unsigned x)
{
    float d[4] = {0, 0, 0, 0};
    for (int i = 0; i < iters; ++i) {
#pragma unroll
        for (int u = 0; u < 8; ++u)
            asm volatile("mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32 "
                         "{%0,%1,%2,%3}, {%4,%4,%4,%4}, {%4,%4}, {%0,%1,%2,%3};"
                         : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]) : "r"(x));
    }
    size_t t = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    for (int j = 0; j < 4; ++j) out[t * 4 + j] = __float_as_uint(d[j]);
}

__global__ void k_f16acc(unsigned* out, int iters, unsigned x)
{
    unsigned d0 = 0, d1 = 0;
    for (int i = 0; i < iters; ++i) {
#pragma unroll
        for (int u = 0; u < 8; ++u)
            asm volatile("mma.sync.aligned.m16n8k16.row.col.f16.f16.f16.f16 "
                         "{%0,%1}, {%2,%2,%2,%2}, {%2,%2}, {%0,%1};"
                         : "+r"(d0), "+r"(d1) : "r"(x));
    }
    size_t t = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    out[t * 2] = d0;
    out[t * 2 + 1] = d1;
}

__global__ void k_bf16acc(unsigned* out, int iters, unsigned x)
{
    float d[4] = {0, 0, 0, 0};
    for (int i = 0; i < iters; ++i) {
#pragma unroll
        for (int u = 0; u < 8; ++u)
            asm volatile("mma.sync.aligned.m16n8k16.row.col.f32.bf16.bf16.f32 "
                         "{%0,%1,%2,%3}, {%4,%4,%4,%4}, {%4,%4}, {%0,%1,%2,%3};"
                         : "+f"(d[0]), "+f"(d[1]), "+f"(d[2]), "+f"(d[3]) : "r"(x));
    }
    size_t t = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
    for (int j = 0; j < 4; ++j) out[t * 4 + j] = __float_as_uint(d[j]);
}

static double bench(void (*k)(unsigned*, int, unsigned), unsigned* out, int iters)
{
    cudaEvent_t a, b;
    cudaEventCreate(&a);
    cudaEventCreate(&b);
    k<<<1024, 256>>>(out, iters, 0x3c003c00u);
    cudaDeviceSynchronize();
    float best = 1e9f;
    for (int r = 0; r < 5; ++r) {
        cudaEventRecord(a);
        k<<<1024, 256>>>(out, iters, 0x3c003c00u);
        cudaEventRecord(b);
        cudaEventSynchronize(b);
        float ms = 0;
        cudaEventElapsedTime(&ms, a, b);
        if (ms < best) best = ms;
    }
    double mmas = 1024.0 * (256.0 / 32.0) * 8.0 * double(iters);
    double flops = mmas * 4096.0;
    return flops / (double(best) * 1e-3) / 1e12;
}

int main()
{
    cudaDeviceProp p;
    cudaGetDeviceProperties(&p, 0);
    printf("device: %s  SMs=%d\n", p.name, p.multiProcessorCount);

    unsigned* out;
    cudaMalloc(&out, 1024ull * 256ull * 8ull * sizeof(unsigned));
    const int iters = 131072;

    // Warm up so the SM clock has ramped before the first measurement.
    bench(k_f16acc, out, 32768);

    printf("f32acc  %.2f TFLOPS\n", bench(k_f32acc, out, iters));
    printf("f16acc  %.2f TFLOPS\n", bench(k_f16acc, out, iters));
    printf("bf16    %.2f TFLOPS\n", bench(k_bf16acc, out, iters));
    return 0;
}
