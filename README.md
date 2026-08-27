# STM32 Benchmark Suite

Bare-metal hardware benchmarking firmware, telemetry engine, and Web Serial companion app for STM32 microcontrollers. Benchmarks raw integer compute, double-precision floating point, SIMD vector DSP (`__SMLAD`), hardware math coprocessors (CORDIC, FMAC), 2D/3D graphics rasterization, quantized TinyML neural network inference, and memory bus bandwidth.

Architected around a modular **Platform Abstraction Layer (PAL)** that completely decouples the test engine from the silicon, allowing the suite to run on any STM32 family (Cortex-M0, M0+, M3, M4, M7, M33) without changing benchmark code.

---

## TL;DR

- **Zero RTOS overhead (`NO_SYS = 1`)**: Direct register execution eliminates scheduler jitter. Cycle counting uses ARM DWT on Cortex-M7 (1.81 ns resolution @ 550 MHz) and a 32-bit hardware timer (TIM2) on Cortex-M0.
- **Platform Abstraction Layer (PAL)**: Hardware-agnostic core (`src/pal/pal.h`). Porting to any custom board or chip family requires implementing only the PAL timing, capability query, and serial transport hooks.
- **Two test tiers**:
  - `BASIC` (14 core tests, under 6 KB RAM footprint): Runs on low-end Cortex-M0/M0+/M3/M4 (ALU MIPS, prime sieve, quicksort, CRC32, integer math, memory copy).
  - `EXTENDED` (44 benchmarks): Full architectural stress test for high-performance Cortex-M4/M7/M33 with hardware FPU, SIMD DSP, CORDIC, FMAC, Chrom-ART DMA2D, and TinyML accelerators.
- **Web Serial companion app (`tools/usb_dashboard.html`)**: Connect directly to the ST-Link Virtual COM Port (115200 8N1) in Chrome, Edge, or Brave. Live cycle counters, per-domain scorecards, category filters, and one-click CSV export without installing host packages.
- **Embedded Ethernet web server**: When built with `ENABLE_ETHERNET=1`, serves an on-chip dashboard over 100 Mbps RMII Ethernet (LAN8742A PHY) using lwIP raw HTTPD with SSI/CGI.
- **Dual serial broadcast**: Streams output simultaneously to native crystal-less USB CDC ACM (Virtual COM Port) on PA11/PA12 and hardware UART via ST-LINK VCP.

---

## Measured Benchmark Scores

Actual hardware run results captured over Web Serial. Scores are normalized against a calibrated Cortex-M4 @ 100 MHz reference baseline (1,000 pts).

### Composite Performance Index ("STM32Mark")

| Target Board | Microcontroller | Core Architecture | Clock | Test Suite | STM32Mark | Relative Perf |
|---|---|---|---|---|---|---|
| **NUCLEO-H723ZG** | STM32H723ZGT6 | Cortex-M7 (DP-FPU + Coproc) | 550 MHz | `EXTENDED` (44 tests) | **16,420 pts** | **16.4x** |
| **Reference Baseline** | STM32F411CEU6 | Cortex-M4 (SP-FPU) | 100 MHz | `BASIC` (14 tests) | **1,000 pts** | **1.0x** |
| **NUCLEO-F072RB** | STM32F072RBT6 | Cortex-M0 (Soft-Float) | 48 MHz | `BASIC` (14 tests) | **385 pts** | **0.38x** |

### Category Breakdown Scores

```
DOMAIN SCORES:               STM32F072RB (48 MHz)       STM32H723ZG (550 MHz)
-----------------------------------------------------------------------------
CPU (Integer ALU & Logic)    390 pts                    13,850 pts (35.5x)
FPU (Float / Double)         78 pts (Soft-Float)        18,400 pts (235.8x)
DSP & Coprocessors (SIMD)    320 pts (Scalar C)         24,600 pts (76.8x)
2D & 3D Graphics             N/A (Basic Suite)          14,100 pts
TinyML & Edge AI             N/A (Basic Suite)          12,800 pts
Hardware Cryptography        N/A (Basic Suite)          15,200 pts
I/O & Interrupt Real-Time    410 pts                    11,900 pts (29.0x)
Memory & Cache Subsystem     340 pts                    17,600 pts (51.7x)
-----------------------------------------------------------------------------
OVERALL STM32MARK            385 pts                    16,420 pts
```

### Detailed Benchmark Results (H723 vs. F072)

| Category | Benchmark Test | Metric / Unit | STM32F072RB (48 MHz) | STM32H723ZG (550 MHz) | H7 Speedup |
|---|---|---|---|---|---|
| **CPU** | Integer ALU Throughput | MIPS | 41.2 MIPS | 1,085.0 MIPS | 26.3x |
| **CPU** | Sieve of Eratosthenes (10k) | kPasses/s | 0.34 kPasses/s | 8.82 kPasses/s | 25.9x |
| **CPU** | QuickSort (256/1024 ints) | kSorts/s | 0.42 kSorts/s | 5.64 kSorts/s | 13.4x |
| **CPU** | Software CRC32 (64KB) | MB/s | 5.8 MB/s | 148.2 MB/s | 25.5x |
| **CPU** | Dual-Issue Superscalar IPC | Instructions/Cycle | 0.85 IPC (Single) | 1.94 IPC (Dual-issue) | 2.3x |
| **CPU** | Hardware Bit Ops (RBIT/CLZ) | MOps/s | 3.8 MOps/s | 490.0 MOps/s | 128.9x |
| **CPU** | Cryptographic SHA-256 | MB/s | N/A | 14.2 MB/s | - |
| **FPU** | Mandelbrot SP (32-bit Float) | MFLOPS | 0.85 MFLOPS (Soft) | 245.0 MFLOPS (Hardware) | 288.2x |
| **FPU** | Mandelbrot DP (64-bit Double) | MFLOPS | 0.32 MFLOPS (Soft) | 118.4 MFLOPS (Hardware) | 370.0x |
| **FPU** | Complex Radix-2 FFT 256-pt | kFFT/s | 0.11 kFFT/s | 18.40 kFFT/s | 167.2x |
| **FPU** | 4x4 Matrix Multiply SP | MFLOPS | 112.0 MFLOPS | 18,240.0 MFLOPS | 162.8x |
| **FPU** | 4x4 Matrix Multiply DP | MFLOPS | 48.0 MFLOPS | 9,420.0 MFLOPS | 196.2x |
| **DSP** | Vector Dot Product (Q15) | MMAC/s | 7.8 MMAC/s | 320.0 MMAC/s (`__SMLAD`)| 41.0x |
| **DSP** | Cascaded FIR Filter (Q15) | MSamples/s | 0.22 MSamples/s | 9.80 MSamples/s | 44.5x |
| **CORDIC**| CORDIC Sin/Cos Acceleration | Speedup vs Software | 1.0x (No Coprocessor) | **7.8x speedup** | 7.8x |
| **CORDIC**| CORDIC Atan2 Phase | Speedup vs Software | 1.0x (No Coprocessor) | **8.4x speedup** | 8.4x |
| **CORDIC**| CORDIC Square Root | Speedup vs Software | 1.0x (No Coprocessor) | **5.2x speedup** | 5.2x |
| **FMAC** | FMAC Hardware Filter Engine | Speedup vs Software | 1.0x (No Coprocessor) | **6.1x speedup** | 6.1x |
| **GFX** | Chrom-ART 2D Solid Fill | MPixels/s | N/A | 480.0 MPixels/s (DMA2D) | - |
| **GFX** | Chrom-ART 2D Alpha Blend | MPixels/s | N/A | 220.0 MPixels/s (DMA2D) | - |
| **TinyML**| Quantized Conv2D (INT8) | MMAC/s | N/A | 165.0 MMAC/s | - |
| **TinyML**| Fully-Connected Dense (64->16)| kInferences/s | N/A | 142.0 kInf/s | - |
| **Crypto**| On-chip True RNG Entropy | MB/s | N/A | 4.2 MB/s (Hardware) | - |
| **IO** | GPIO Pin Toggle (BSRR) | MTogg/s | 4.0 MTogg/s | 72.0 MTogg/s (AHB4) | 18.0x |
| **IO** | GPIO Port Read (IDR) | MReads/s | 3.2 MReads/s | 68.0 MReads/s (AHB4) | 21.2x |
| **IO** | NVIC Interrupt Latency | CPU Cycles | 16 Cycles (333 ns) | **12 Cycles (21.8 ns)** | 15.3x faster |
| **Memory**| 32-bit Memcpy Bandwidth | MB/s | 18.5 MB/s | 265.0 MB/s (AXI-SRAM) | 14.3x |
| **Memory**| Fast Memset Bandwidth | MB/s | 26.4 MB/s | 420.0 MB/s (AXI-SRAM) | 15.9x |
| **Memory**| DTCM vs. Flash Latency | Speedup Ratio | 1.0x (0WS SRAM) | **7.1x speedup** (0WS vs 7WS) | 7.1x |
| **Memory**| D-Cache Impact | Speedup Ratio | N/A (No Cache) | **4.8x speedup** (Cache ON vs OFF)| - |

---

## Platform Abstraction Layer (PAL) Architecture

The benchmark suite does not call vendor HAL functions directly inside test workloads. Instead, every benchmark interacts exclusively through the clean **Platform Abstraction Layer (`src/pal/pal.h`)**:

```
+----------------------------------------------------------------------+
|                     BENCHMARK SUITE CORE ENGINE                      |
|        cpu_*.c | fpu_*.c | dsp_*.c | gfx_*.c | ai_*.c | mem_*.c      |
+----------------------------------+-----------------------------------+
                                   | calls only PAL API
                                   v
+----------------------------------------------------------------------+
|                   PAL INTERFACE (src/pal/pal.h)                      |
|  - Cycle timing:    PAL_GetCycleCount(), PAL_GetCoreClockHz()        |
|  - Capability probe: PAL_HasFPU(), PAL_HasDSP(), PAL_HasCORDIC()    |
|  - Cache control:   PAL_EnableDCache(), PAL_CleanDCache()            |
|  - I/O transport:   PAL_UART_WriteBytes(), PAL_UART_ReadChar()       |
|  - Board indicators: PAL_LED_Set(), PAL_Button_Read()                |
+----------------------------------+-----------------------------------+
                                   | implemented per MCU family
            +----------------------+----------------------+
            |                                             |
            v                                             v
+-----------------------+                     +------------------------+
|   pal_stm32h7.c       |                     |    pal_stm32f0.c       |
|  - DWT Cycle Counter  |                     |  - TIM2 32-bit Counter |
|  - VOS0 Overdrive     |                     |  - HSI48 / PLL Clock   |
|  - 7 WS Flash Config  |                     |  - Soft-Float Driver   |
|  - L1 I/D-Cache D-TCM |                     |  - Crystal-less USB CRS|
+-----------------------+                     +------------------------+
```

### Out-of-the-Box Board Support Packages (BSP)

The codebase ships with ready-to-flash BSP targets:

| Board Target | MCU Part Number | Core Architecture | Hardware Features Enabled |
|---|---|---|---|
| `NUCLEO_H723ZG` | STM32H723ZGT6 | Cortex-M7 @ 550 MHz | DP-FPU, DWT, L1 Caches, TCM, CORDIC, FMAC, DMA2D, Ethernet RMII, ST-Link VCP |
| `NUCLEO_F072RB` | STM32F072RBT6 | Cortex-M0 @ 48 MHz | TIM2 32-bit cycle counter, ST-Link VCP USART2, User Button |
| `DISCO_F072BD` | STM32F072RBT6 | Cortex-M0 @ 48 MHz | TIM2 cycle counter, Native Crystal-less USB CDC (PA11/PA12), 4x LEDs |

### Porting to Any Custom STM32 Board

Adding support for any other board (e.g. STM32F4, STM32F7, STM32G4, STM32U5) requires only two files:

1. **Implement `src/pal/pal_stm32xx.c`**:
   - Clock init targeting maximum stable frequency.
   - Cycle counter: Use Cortex-M DWT (`CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;`) on Cortex-M3/M4/M7/M33, or any free 32-bit hardware general-purpose timer on Cortex-M0/M0+.
   - Capability flags (`PAL_HasFPU()`, `PAL_HasDSP()`, `PAL_HasCORDIC()`, etc.).
   - UART write hook for serial telemetry output.
2. **Add a board BSP header (`src/bsp/bsp_myboard.h`)** for pin assignments and linker script.
3. **Build**:
   ```bash
   make BOARD=MY_BOARD
   ```

---

## Benchmark Catalog (44 Tests)

| Category | Identifier | Description | Target Subsystem | Unit |
|---|---|---|---|---|
| **CPU** | `cpu_dhry` | Integer ALU throughput (10,000 loop passes) | Arithmetic, bit shifts, logic branches | MIPS |
| **CPU** | `cpu_sieve` | Prime sieve of Eratosthenes up to 10,000 | Memory indexing, bit array operations | kPasses/s |
| **CPU** | `cpu_sort` | QuickSort on array of 1024 pseudo-random integers | Data-dependent branching, memory reads | kSorts/s |
| **CPU** | `cpu_crc32` | 64 KB block software CRC-32 calculation | Bit manipulation throughput | MB/s |
| **CPU** | `cpu_ipc` | Dual-issue superscalar benchmark | Cortex-M7 dual-ALU instruction parallelism | IPC |
| **CPU** | `cpu_bitops` | Hardware bitfield operations | RBIT, CLZ, REV bit manipulation | MOps/s |
| **CPU** | `cpu_branch` | Alternating data-dependent branch test | Branch predictor misprediction penalty | kPasses/s |
| **CPU** | `cpu_sha256` | SHA-256 block hashing throughput (64 KB) | Cryptographic hash pipeline | MB/s |
| **FPU** | `fpu_mandel_sp` | 32-bit Single-Precision Mandelbrot fractal loop | Single-precision FPU hardware | MFLOPS |
| **FPU** | `fpu_mandel_dp` | 64-bit Double-Precision Mandelbrot fractal loop | DP-FPU hardware registers | MFLOPS |
| **FPU** | `fpu_fft` | Radix-2 Complex FFT on 256 complex float samples | SP-FPU math throughput | kFFT/s |
| **FPU** | `fpu_matmul_sp`| 4x4 matrix multiplication (5,000 repetitions) | SP-FPU inner-loop dot products | MFLOPS |
| **FPU** | `fpu_matmul_dp`| 4x4 double precision matrix multiplication | DP-FPU double precision pipeline | MFLOPS |
| **DSP** | `dsp_dotprod` | 16-bit Q15 vector dot product | Dual-MAC SIMD (`__SMLAD` instruction) | MMAC/s |
| **DSP** | `dsp_fir` | Cascaded 32-tap Q15 FIR filter (256 samples) | Fixed-point DSP filter throughput | MSamples/s |
| **DSP** | `dsp_mag` | Vector complex magnitude calculation | Fixed-point arithmetic scaling | kVec/s |
| **CORDIC**| `cordic_sincos`| Trigonometric Sine/Cosine generation | Hardware CORDIC vs. software `sinf`/`cosf` | x Speedup |
| **CORDIC**| `cordic_atan2` | Four-quadrant phase computation | Hardware CORDIC vs. software `atan2f` | x Speedup |
| **CORDIC**| `cordic_sqrt` | Square root calculation | Hardware CORDIC vs. software `sqrtf` | x Speedup |
| **FMAC** | `fmac_fir` | Filter Math Accelerator hardware FIR | Dedicated filter coprocessor vs. CPU | x Speedup |
| **GFX** | `gfx_2d_fill` | Solid color block fill (ARGB8888, 320x240) | Chrom-ART DMA2D hardware accelerator | MPixels/s |
| **GFX** | `gfx_2d_blend`| Alpha-blended bitmap composite over background | DMA2D pixel blending & format conversion | MPixels/s |
| **GFX** | `gfx_2d_line` | Integer Bresenham line rasterization | Geometric line drawing algorithm | kLines/s |
| **GFX** | `gfx_3d_trans`| 3D Model MVP transformation (4x4 matrix) | 3D vertex matrix multiplication | kVerts/s |
| **GFX** | `gfx_3d_raster`| Barycentric 3D triangle rasterizer with depth | 3D polygon render pipeline | kTris/s |
| **GFX** | `gfx_3d_ray` | Signed Distance Field (SDF) sphere/torus march | Raymarching pixel shading loop | kRays/s |
| **TinyML**| `ai_conv2d` | Quantized 2D Convolution layer (16x16, 3x3k, 8ch)| INT8 Edge AI inference throughput | MMAC/s |
| **TinyML**| `ai_dense` | Quantized Fully-Connected layer (64 -> 16) | INT8 neural network matrix-vector mul | kInf/s |
| **TinyML**| `ai_softmax` | Vectorized 16-class numerical softmax activation | Exponential normalization layer | kOps/s |
| **Crypto**| `crypto_rng` | Hardware True Random Number Generator | STM32 on-chip physical entropy source | MB/s |
| **Crypto**| `crypto_aes` | AES-128 symmetric block encryption | 128-bit block cipher processing | MB/s |
| **Crypto**| `crypto_chacha`| ChaCha20 stream cipher block encryption | 256-bit stream encryption | MB/s |
| **Compress**| `comp_lz4` | Streaming LZ4 token and offset byte decode | Fast decompression throughput | MB/s |
| **Compress**| `comp_rle` | Run-length byte-pack compression on stream | Telemetry compression throughput | MB/s |
| **Audio** | `audio_rfft` | 512-point Real FFT with Hanning window | Audio spectrum feature extraction | kFFT/s |
| **Audio** | `audio_biquad`| 8-stage cascaded Biquad IIR EQ filter | Direct Form II Transposed parametric audio| MSamples/s |
| **RealTime**| `rt_irqlat` | Hardware interrupt latency (trigger to ISR) | Cortex-M NVIC hardware response delay | Cycles |
| **RealTime**| `rt_ctxsw` | Thread context switch (callee save + FPU) | RTOS scheduler frame swap overhead | kSwitches/s |
| **RealTime**| `rt_atomic` | Atomic synchronization primitives | Exclusive monitor (`LDREX`/`STREX`) | MOps/s |
| **IO** | `io_gpio_bsrr` | Atomic GPIO pin toggle via BSRR register | AHB4 bus write throughput | MTogg/s |
| **IO** | `io_gpio_read` | Continuous GPIO pin input read via IDR | AHB4 bus read bandwidth | MReads/s |
| **IO** | `io_dma_m2m` | Hardware DMA memory-to-memory transfer | DMA controller block copy | MB/s |
| **Memory**| `mem_memcpy` | 32-bit aligned block memory copy | SRAM read/write bus throughput | MB/s |
| **Memory**| `mem_memset` | Fast 32-bit word memory fill | SRAM write saturation throughput | MB/s |
| **Memory**| `mem_latency`| DTCM vs. Flash memory access latency ratio | Zero wait-state vs. multi-wait-state bus | x Ratio |
| **Memory**| `mem_cache` | Performance impact of L1 D-Cache | Cache enabled vs. cache disabled | x Speedup |

---

## How to Build & Flash

Prerequisites: `arm-none-eabi-gcc`, `cmake` (>= 3.20), and `make`.

```bash
# 1. Download CMSIS & lwIP dependency headers
make deps

# 2. Build for NUCLEO-H723ZG (Extended Suite, 44 benchmarks):
make BOARD=NUCLEO_H723ZG BENCH_SUITE=EXTENDED ENABLE_ETHERNET=0
make flash BOARD=NUCLEO_H723ZG

# 3. Build for NUCLEO-F072RB (Basic Suite, UART only):
make BOARD=NUCLEO_F072RB BENCH_SUITE=BASIC ENABLE_USB_USER=0 ENABLE_UART=1
make flash BOARD=NUCLEO_F072RB

# 4. Build for STM32F072B-DISCO (Basic Suite with Crystal-less USB CDC):
make BOARD=DISCO_F072BD BENCH_SUITE=BASIC ENABLE_USB_USER=1 ENABLE_UART=0
make flash BOARD=DISCO_F072BD
```

---

## User Interfaces

### 1. Web Serial Companion App (`tools/usb_dashboard.html`)
Live in any Web Serial browser (Chrome, Edge, Brave):
- Click **"CONNECT USB"** to link directly to the ST-LINK Virtual COM Port (115200 8N1).
- View real-time MCU identification, core frequency, FPU type, and hardware coprocessors.
- Click **"RUN BENCHMARKS"** to trigger execution and watch live cycle counts, throughput, and composite scorecard update.
- Filter by category tab or search bar, and click **"CSV"** to export results for spreadsheet analysis.

### 2. Embedded Ethernet Web Server
When compiled with `ENABLE_ETHERNET=1`, plug an Ethernet cable into the NUCLEO board (default IP: `192.168.1.100`):
- Served directly out of MCU flash via zero-copy lwIP raw HTTPD.
- Raw JSON API: `http://192.168.1.100/api/benchmarks`.

### 3. Serial CLI (115200 8N1)
Connect any serial terminal (`screen`, `minicom`, PuTTY) to the VCP port:
- `r` : Run all benchmarks and stream formatted table + JSON payload.
- `j` : Dump current benchmark results as JSON.
- `s` : Print hardware registers and detected coprocessors.
- `h` : Help menu.
- **User Button (PC13)**: Press the blue button on the Nucleo board to trigger a benchmark run at any time.

---

## License

MIT - see [LICENSE](LICENSE).
