# STM32 Benchmark Suite

Bare-metal hardware benchmarking firmware and telemetry engine for STM32 microcontrollers. Benchmarks raw compute, SIMD vector math, memory hierarchy bandwidth, hardware coprocessors (CORDIC, FMAC, DMA2D), and interrupt latency on **STM32H723ZG** (Cortex-M7 @ 550 MHz) and **STM32F072RB** (Cortex-M0 @ 48 MHz).

---

## TL;DR

- **Bare-metal execution (`NO_SYS = 1`)**: Zero RTOS overhead or scheduler jitter. Cycle measurements run via ARM DWT on Cortex-M7 (1.81 ns resolution) and a 32-bit hardware timer (TIM2) on Cortex-M0.
- **Two test tiers**:
  - `BASIC` (14 core tests, under 6 KB RAM footprint): ALU MIPS, prime sieve, quicksort, CRC32, integer math, memory throughput. Runs on low-end Cortex-M0/M3/M4.
  - `EXTENDED` (44 benchmarks): Adds double-precision FPU Mandelbrot/MatMul, Q15 SIMD DSP (`__SMLAD`), CORDIC trigonometric acceleration, FMAC IIR/FIR filters, Chrom-ART DMA2D pixel blending, quantized INT8 Conv2D/Dense TinyML layers, hardware RNG/AES/ChaCha20, and ITCM vs DTCM vs AXI latency.
- **Web Serial companion app (`tools/usb_dashboard.html`)**: Connect directly to the ST-Link Virtual COM Port (115200 baud) in Chrome/Edge/Brave. Live telemetry, real-time scorecards, category filters, and CSV export without installing host software.
- **On-chip HTTP server**: When compiled with `ENABLE_ETHERNET=1`, serves an embedded dashboard over 100 Mbps RMII Ethernet (LAN8742A PHY) using a zero-copy lwIP raw HTTPD stack directly from MCU flash.
- **Dual serial broadcast**: Supports native crystal-less USB CDC ACM (Virtual COM Port) on PA11/PA12 and hardware UART via ST-LINK VCP simultaneously.

---

## Supported Hardware

| Target Board | Core Architecture | Clock | SRAM / TCM | Flash | Primary Benchmarks |
|---|---|---|---|---|---|
| **NUCLEO-H723ZG** | ARM Cortex-M7 (DP-FPU) | 550 MHz | 564 KB + 64K ITCM + 128K DTCM | 1024 KB | Extended (44 tests, CORDIC, FMAC, DMA2D, TinyML) |
| **NUCLEO-F072RB** | ARM Cortex-M0 (Soft-Float) | 48 MHz | 16 KB | 128 KB | Basic (14 tests, ALU, Sieve, QSort, Memcpy) |

---

## Architecture

```
+-----------------------------------------------------------------------+
|                      STM32 Benchmark Application                      |
+-------------+-------------+-------------+--------------+--------------+
|  CPU Bench  |  FPU Bench  |  DSP Bench  | CORDIC/FMAC  | Memory Bench |
|  - ALU MIPS |  - SP / DP  |  - SMLAD    |  - Sin / Cos | - Memcpy AXI |
|  - Sieve    |  - 256 FFT  |  - Q15 FIR  |  - Atan2     | - Memset     |
|  - QSort    |  - MatMul   |  - MagVec   |  - Sqrt/FIR  | - ITCM/DTCM  |
+-------------+-------------+-------------+--------------+--------------+
|            Platform Abstraction Layer (PAL) & BSP Engine              |
|      - STM32H7 (DWT @ 550MHz, L1 Cache, TCM, AXI, DMA1/2, MDMA)       |
|      - STM32F0 (TIM2 32-bit Timer @ 48MHz, Soft-Float, 16KB RAM)     |
+-------------------------------------+---------------------------------+
|       Ethernet lwIP Transport       |     USB / Serial Transport      |
|       - STM32H7 MAC (RMII)          |     - ST-LINK VCP (USART2/3)    |
|       - LAN8742A PHY Driver         |     - Web Serial Dashboard      |
|       - lwIP 2.2.0 RAW HTTPD        |     - JSON Telemetry Stream     |
+-------------------------------------+---------------------------------+
```

---

## Benchmark Catalog

| Category | Benchmark | Workload Details | Unit |
|---|---|---|---|
| **CPU** | Integer ALU Throughput | 32-bit arithmetic, bit shifts, logic branches | MIPS |
| **CPU** | Sieve of Eratosthenes | Prime generator up to 10k, memory & bit array | kPasses/s |
| **CPU** | QuickSort (1024 ints) | Data-dependent branch prediction & memory ops | kSorts/s |
| **CPU** | Software CRC32 (64KB) | Bit manipulation throughput on 64KB block | MB/s |
| **CPU** | Dual-Issue Superscalar | Cortex-M7 dual-ALU instruction parallelism | IPC |
| **CPU** | Hardware Bit Ops | RBIT, CLZ, REV bitfield manipulation | MOps/s |
| **CPU** | Cryptographic SHA-256 | SHA-256 block hashing throughput on 64KB | MB/s |
| **FPU** | Mandelbrot (SP Float) | Single-Precision FPU 32-bit fractal loop | MFLOPS |
| **FPU** | Mandelbrot (DP Double)| Double-Precision DP-FPU 64-bit fractal loop | MFLOPS |
| **FPU** | Complex FFT 256-pt | Radix-2 single-precision complex FFT | kFFT/s |
| **FPU** | Matrix Multiply (SP) | 4x4 Single Precision matrix mul (5,000x) | MFLOPS |
| **FPU** | Matrix Multiply (DP) | 4x4 Double Precision matrix mul (5,000x) | MFLOPS |
| **DSP** | Vector Dot Product (Q15)| Cortex-M7 dual-MAC SIMD (`__SMLAD`) | MMAC/s |
| **DSP** | FIR Filter (32-tap Q15) | 256 samples filtered with DSP instructions | MSamples/s |
| **DSP** | Vector Magnitude (Q15) | Fixed-point vector complex magnitude | kVec/s |
| **CORDIC**| CORDIC Sin/Cos | Hardware CORDIC vs. software `sinf`/`cosf` | x Speedup |
| **CORDIC**| CORDIC Atan2 | Hardware CORDIC Phase vs. software `atan2f` | x Speedup |
| **CORDIC**| CORDIC Square Root | Hardware CORDIC vs. software `sqrtf` | x Speedup |
| **FMAC** | FMAC Hardware FIR | Dedicated hardware filter coprocessor vs CPU | x Speedup |
| **GFX** | 2D Rect Fill (DMA2D) | Chrom-ART DMA2D solid color fill (ARGB8888) | MPixels/s |
| **GFX** | 2D Alpha Blend (DMA2D)| DMA2D pixel blending & format conversion | MPixels/s |
| **GFX** | Bresenham 2D Line Draw| Integer geometric line rasterization | kLines/s |
| **GFX** | 3D MVP Vertex Transform| 3D vertex rotation, 4x4 matrix projection | kVerts/s |
| **GFX** | 3D Triangle Rasterizer | Barycentric coordinate rasterizer & depth testing | kTris/s |
| **GFX** | 3D SDF Raymarching | Sphere & torus signed distance field raymarch | kRays/s |
| **TinyML**| Quantized Conv2D (int8)| 2D Convolution layer (16x16, 3x3k, 8ch, ReLU) | MMAC/s |
| **TinyML**| Dense Layer (64->16) | Quantized fully-connected neural network layer | kInf/s |
| **TinyML**| Softmax Activation | Vectorized 16-class numerical softmax | kOps/s |
| **Crypto**| Hardware True RNG | STM32H7 on-chip physical entropy generator | MB/s |
| **Crypto**| AES-128 Block Cipher | Symmetric 128-bit block encryption | MB/s |
| **Crypto**| ChaCha20 Stream Cipher| 256-bit stream cipher block encryption | MB/s |
| **Compress**| LZ4 Decompression | Streaming LZ4 token/offset byte decode | MB/s |
| **Compress**| RLE Byte-Pack Compress| Run-length encoding on telemetry stream | MB/s |
| **Audio** | Real FFT 512-pt Window | 512-pt real FFT with Hanning window | kFFT/s |
| **Audio** | 8-Stage Biquad IIR EQ | Direct Form II Transposed parametric audio | MSamples/s |
| **RealTime**| NVIC Interrupt Latency | Hardware cycle delay from trigger to ISR | Cycles |
| **RealTime**| RTOS Context Switch | Full Cortex-M7 callee + FPU frame switch | kSwitches/s |
| **RealTime**| Atomic Exclusive Monitor| LDREX/STREX spinlock sync primitives | MOps/s |
| **IO** | GPIO Pin Toggle (BSRR) | AHB4 atomic pin set/reset toggle speed | MTogg/s |
| **IO** | GPIO Input Read (IDR) | Continuous AHB4 port read bandwidth | MReads/s |
| **IO** | DMA Memory-to-Memory | DMA1 32-bit hardware block copy | MB/s |
| **Memory**| Memcpy Bandwidth | 32-bit aligned block memory copy on AXI-SRAM | MB/s |
| **Memory**| Memset Bandwidth | Fast memory fill write throughput | MB/s |
| **Memory**| DTCM vs. Flash Latency| 0-wait state DTCM vs. 7-wait state Flash | x Ratio |
| **Memory**| D-Cache Impact | Performance speedup with D-Cache ON vs OFF | x Speedup |

---

## How to Build & Flash

Prerequisites: `arm-none-eabi-gcc`, `cmake`, and `make`.

```bash
# 1. Fetch CMSIS & lwIP dependencies
make deps

# 2. Build for NUCLEO-H723ZG (Extended Suite):
make BOARD=NUCLEO_H723ZG BENCH_SUITE=EXTENDED ENABLE_ETHERNET=0
make flash BOARD=NUCLEO_H723ZG

# 3. Build for NUCLEO-F072RB (Basic Suite):
make BOARD=NUCLEO_F072RB BENCH_SUITE=BASIC ENABLE_USB_USER=1 ENABLE_UART=1
make flash BOARD=NUCLEO_F072RB
```

---

## Interfaces

### 1. Web Serial Dashboard (`tools/usb_dashboard.html`)
Open [`tools/usb_dashboard.html`](tools/usb_dashboard.html) in Chrome, Edge, or Brave:
- Click **"CONNECT USB"** and choose the ST-LINK Virtual COM Port (115200 baud).
- Click **"RUN ALL"** to stream test progress, cycle counts, MIPS/MFLOPS metrics, and composite score.
- Export results directly to CSV.

### 2. Embedded Ethernet Web Server
Connect an Ethernet cable to the NUCLEO board's RJ-45 jack (default IP: `192.168.1.100`):
- Uses lwIP Server-Side Includes (SSI) and CGI for in-browser telemetry without host software.
- Raw JSON API: `http://192.168.1.100/api/benchmarks`.

### 3. Serial CLI (115200 8N1)
Connect any serial terminal (`screen`, `minicom`, PuTTY) to the VCP port:
- `r` : Run all benchmarks and output formatted table + JSON stream.
- `j` : Dump current results as JSON.
- `s` : Print hardware specs and register telemetry.
- `h` : Help menu.
- **User Button (PC13)**: Press the blue button to trigger a benchmark run manually.

---

## License

MIT - see [LICENSE](LICENSE).
