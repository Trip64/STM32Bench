/* 2D Graphics (DMA2D Chrom-ART) and 3D Graphics Pipeline Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include "stm32h7xx.h"
#include <math.h>
#include <string.h>

extern volatile uint32_t g_sink;

#define FRAME_WIDTH   64
#define FRAME_HEIGHT  64
#define FRAME_PIXELS  (FRAME_WIDTH * FRAME_HEIGHT)

/* Internal framebuffers allocated in AXI-SRAM */
static uint32_t s_framebuf_dst[FRAME_PIXELS];
static uint32_t s_framebuf_src[FRAME_PIXELS];
static float    s_depth_buf[FRAME_PIXELS];

/* ── 2D GRAPHICS ──────────────────────────────────────────────────────── */

/* 1. 2D Rectangle Fill (DMA2D Hardware Acceleration vs CPU) */
void Bench_2D_Fill(BenchResult *res)
{
    const int iterations = 100;
    bool has_dma2d = PAL_HasDMA2D();

    if (has_dma2d) {
        RCC->AHB3ENR |= RCC_AHB3ENR_DMA2DEN;
        __DSB();
    }

    BENCH_START();
    if (has_dma2d) {
        /* DMA2D Mode 3: Register-to-memory (solid color fill) */
        DMA2D->CR = (3U << DMA2D_CR_MODE_Pos);
        DMA2D->OPFCCR = 0; /* ARGB8888 */
        DMA2D->OOR = 0;
        DMA2D->NLR = (FRAME_WIDTH << DMA2D_NLR_PL_Pos) | (FRAME_HEIGHT << DMA2D_NLR_NL_Pos);

        for (int i = 0; i < iterations; i++) {
            DMA2D->OMAR = (uint32_t)s_framebuf_dst;
            DMA2D->OCOLR = 0xFF00FF00 | (uint32_t)i; /* Solid color */
            DMA2D->CR |= DMA2D_CR_START;
            uint32_t to = 50000;
            while ((DMA2D->CR & DMA2D_CR_START) && --to) {
                if (DMA2D->ISR & (DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) {
                    DMA2D->IFCR = DMA2D_IFCR_CTEIF | DMA2D_IFCR_CCEIF;
                    break;
                }
            }
            if (!to) break;
        }
        res->available = true;
    } else {
        /* Software fallback: 32-bit pixel writes */
        for (int i = 0; i < iterations; i++) {
            uint32_t color = 0xFF00FF00 | (uint32_t)i;
            for (int p = 0; p < FRAME_PIXELS; p++) {
                s_framebuf_dst[p] = color;
            }
        }
        res->available = false;
    }
    g_sink = s_framebuf_dst[0];
    BENCH_STOP();

    float total_pixels = (float)iterations * (float)FRAME_PIXELS;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_pixels / time_sec / 1000000.0f) : 0.0f; /* MPixels/s */
}

/* 2. 2D Alpha Blending (DMA2D Hardware PFC Blending vs CPU) */
void Bench_2D_AlphaBlend(BenchResult *res)
{
    const int iterations = 50;
    bool has_dma2d = PAL_HasDMA2D();

    for (int p = 0; p < FRAME_PIXELS; p++) {
        s_framebuf_src[p] = 0x80FF0000 | (uint32_t)(p & 0xFF); /* 50% alpha red */
        s_framebuf_dst[p] = 0xFF0000FF;                         /* Solid blue */
    }

    if (has_dma2d) {
        RCC->AHB3ENR |= RCC_AHB3ENR_DMA2DEN;
        __DSB();
    }

    BENCH_START();
    if (has_dma2d) {
        /* DMA2D Mode 2: Memory-to-memory with pixel blending */
        DMA2D->CR = (2U << DMA2D_CR_MODE_Pos);
        DMA2D->OPFCCR = 0; /* ARGB8888 output */
        DMA2D->OOR = 0;
        DMA2D->NLR = (FRAME_WIDTH << DMA2D_NLR_PL_Pos) | (FRAME_HEIGHT << DMA2D_NLR_NL_Pos);

        /* Foreground: Source */
        DMA2D->FGMAR = (uint32_t)s_framebuf_src;
        DMA2D->FGOR = 0;
        DMA2D->FGPFCCR = 0; /* ARGB8888, use original alpha */

        /* Background: Destination */
        DMA2D->BGMAR = (uint32_t)s_framebuf_dst;
        DMA2D->BGOR = 0;
        DMA2D->BGPFCCR = 0; /* ARGB8888 */

        for (int i = 0; i < iterations; i++) {
            DMA2D->OMAR = (uint32_t)s_framebuf_dst;
            DMA2D->CR |= DMA2D_CR_START;
            uint32_t to = 50000;
            while ((DMA2D->CR & DMA2D_CR_START) && --to) {
                if (DMA2D->ISR & (DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) {
                    DMA2D->IFCR = DMA2D_IFCR_CTEIF | DMA2D_IFCR_CCEIF;
                    break;
                }
            }
            if (!to) break;
        }
        res->available = true;
    } else {
        /* Software alpha blend loop */
        for (int i = 0; i < iterations; i++) {
            for (int p = 0; p < FRAME_PIXELS; p++) {
                uint32_t src = s_framebuf_src[p];
                uint32_t dst = s_framebuf_dst[p];
                uint32_t a = (src >> 24) & 0xFF;
                uint32_t inv_a = 255 - a;

                uint32_t r = (((src >> 16) & 0xFF) * a + ((dst >> 16) & 0xFF) * inv_a) >> 8;
                uint32_t g = (((src >> 8)  & 0xFF) * a + ((dst >> 8)  & 0xFF) * inv_a) >> 8;
                uint32_t b = (((src)       & 0xFF) * a + ((dst)       & 0xFF) * inv_a) >> 8;

                s_framebuf_dst[p] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
        res->available = false;
    }
    g_sink = s_framebuf_dst[0];
    BENCH_STOP();

    float total_pixels = (float)iterations * (float)FRAME_PIXELS;
    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? (total_pixels / time_sec / 1000000.0f) : 0.0f; /* MPixels/s */
}

/* 3. 2D Line Rasterization (Bresenham Line Algorithm) */
static void draw_line_bresenham(int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 >= y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (1) {
        if (x0 >= 0 && x0 < FRAME_WIDTH && y0 >= 0 && y0 < FRAME_HEIGHT) {
            s_framebuf_dst[y0 * FRAME_WIDTH + x0] = color;
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
}

void Bench_2D_Bresenham(BenchResult *res)
{
    const int lines = 2000;

    BENCH_START();
    for (int i = 0; i < lines; i++) {
        int x0 = (i * 37) % FRAME_WIDTH;
        int y0 = (i * 59) % FRAME_HEIGHT;
        int x1 = (i * 83) % FRAME_WIDTH;
        int y1 = (i * 97) % FRAME_HEIGHT;
        draw_line_bresenham(x0, y0, x1, y1, 0xFFFFFFFF);
    }
    g_sink = s_framebuf_dst[lines % FRAME_PIXELS];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)lines / time_sec / 1000.0f) : 0.0f; /* kLines/s */
}

/* ── 3D GRAPHICS PIPELINE ────────────────────────────────────────────── */

typedef struct { float x, y, z, w; } Vec4;
typedef struct { float m[4][4]; } Mat4;

static Vec4 mat4_mul_vec4(const Mat4 *M, const Vec4 *v)
{
    Vec4 out;
    out.x = M->m[0][0]*v->x + M->m[0][1]*v->y + M->m[0][2]*v->z + M->m[0][3]*v->w;
    out.y = M->m[1][0]*v->x + M->m[1][1]*v->y + M->m[1][2]*v->z + M->m[1][3]*v->w;
    out.z = M->m[2][0]*v->x + M->m[2][1]*v->y + M->m[2][2]*v->z + M->m[2][3]*v->w;
    out.w = M->m[3][0]*v->x + M->m[3][1]*v->y + M->m[3][2]*v->z + M->m[3][3]*v->w;
    return out;
}

/* 4. 3D Vertex Transformation & Perspective Projection Pipeline */
void Bench_3D_Transform(BenchResult *res)
{
    const int count = 20000;

    /* 4x4 Combined Model-View-Projection matrix */
    Mat4 MVP = {{
        { 1.81f,  0.00f,  0.00f,  0.00f },
        { 0.00f,  2.41f,  0.00f,  0.00f },
        { 0.00f,  0.00f, -1.02f, -1.00f },
        { 0.00f,  0.00f, -2.02f,  0.00f }
    }};

    Vec4 v = { 1.5f, -2.0f, 5.0f, 1.0f };

    BENCH_START();
    for (int i = 0; i < count; i++) {
        v.x = (float)(i % 100) * 0.1f - 5.0f;
        v.y = (float)(i % 70) * 0.1f - 3.5f;
        v.z = 2.0f + (float)(i % 50) * 0.2f;

        Vec4 clip = mat4_mul_vec4(&MVP, &v);

        /* Perspective divide & Viewport mapping */
        if (clip.w != 0.0f) {
            float inv_w = 1.0f / clip.w;
            float ndc_x = clip.x * inv_w;
            float ndc_y = clip.y * inv_w;
            float screen_x = (ndc_x + 1.0f) * 0.5f * (float)FRAME_WIDTH;
            float screen_y = (1.0f - ndc_y) * 0.5f * (float)FRAME_HEIGHT;
            s_depth_buf[i % FRAME_PIXELS] = screen_x + screen_y;
        }
    }
    g_sink = (uint32_t)s_depth_buf[0];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)count / time_sec / 1000.0f) : 0.0f; /* kVertices/s */
}

/* 5. 3D Triangle Rasterizer with Barycentric Interpolation */
static inline float edge_func(float ax, float ay, float bx, float by, float cx, float cy)
{
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

void Bench_3D_Rasterize(BenchResult *res)
{
    const int triangles = 500;

    for (int i = 0; i < FRAME_PIXELS; i++) {
        s_depth_buf[i] = 1000.0f;
        s_framebuf_dst[i] = 0xFF000000;
    }

    BENCH_START();
    for (int t = 0; t < triangles; t++) {
        float x0 = (float)((t * 19) % (FRAME_WIDTH - 20));
        float y0 = (float)((t * 29) % (FRAME_HEIGHT - 20));
        float x1 = x0 + 18.0f;
        float y1 = y0 + 5.0f;
        float x2 = x0 + 6.0f;
        float y2 = y0 + 16.0f;

        float area = edge_func(x0, y0, x1, y1, x2, y2);
        if (area <= 0.0f) continue;
        float inv_area = 1.0f / area;

        int min_x = (int)x0, max_x = (int)x1;
        int min_y = (int)y0, max_y = (int)y2;
        if (min_x < 0) min_x = 0;
        if (max_x >= FRAME_WIDTH) max_x = FRAME_WIDTH - 1;
        if (min_y < 0) min_y = 0;
        if (max_y >= FRAME_HEIGHT) max_y = FRAME_HEIGHT - 1;

        for (int y = min_y; y <= max_y; y++) {
            for (int x = min_x; x <= max_x; x++) {
                float w0 = edge_func(x1, y1, x2, y2, (float)x, (float)y);
                float w1 = edge_func(x2, y2, x0, y0, (float)x, (float)y);
                float w2 = edge_func(x0, y0, x1, y1, (float)x, (float)y);

                if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                    float z = (w0 * 1.0f + w1 * 2.0f + w2 * 3.0f) * inv_area;
                    int idx = y * FRAME_WIDTH + x;
                    if (z < s_depth_buf[idx]) {
                        s_depth_buf[idx] = z;
                        s_framebuf_dst[idx] = 0xFF0000FF | (uint32_t)t;
                    }
                }
            }
        }
    }
    g_sink = s_framebuf_dst[0];
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)triangles / time_sec / 1000.0f) : 0.0f; /* kTris/s */
}

/* 6. 3D Signed Distance Field (SDF) Raymarching Engine */
static inline float map_sphere_torus(float px, float py, float pz)
{
    /* Sphere at origin radius 1.0 */
    float d_sphere = sqrtf(px*px + py*py + pz*pz) - 1.0f;
    /* Ground plane at y = -1.2 */
    float d_plane = py + 1.2f;
    return (d_sphere < d_plane) ? d_sphere : d_plane;
}

void Bench_3D_Raymarch(BenchResult *res)
{
    const int rays = 4096; /* 64x64 ray grid */
    const int max_steps = 24;
    int hits = 0;

    BENCH_START();
    for (int r = 0; r < rays; r++) {
        float u = ((float)(r % 64) / 32.0f) - 1.0f;
        float v = ((float)(r / 64) / 32.0f) - 1.0f;

        /* Ray origin and normalized direction */
        float ro_x = 0.0f, ro_y = 0.0f, ro_z = -3.0f;
        float rd_len = sqrtf(u*u + v*v + 1.0f);
        float rd_x = u / rd_len, rd_y = v / rd_len, rd_z = 1.0f / rd_len;

        float t = 0.0f;
        for (int step = 0; step < max_steps; step++) {
            float px = ro_x + rd_x * t;
            float py = ro_y + rd_y * t;
            float pz = ro_z + rd_z * t;
            float dist = map_sphere_torus(px, py, pz);

            if (dist < 0.01f) {
                hits++;
                break;
            }
            t += dist;
            if (t > 15.0f) break;
        }
    }
    g_sink = hits;
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)rays / time_sec / 1000.0f) : 0.0f; /* kRays/s */
}
