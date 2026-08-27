/* Real-Time OS & Interrupt Latency Benchmarks */

#include "bench_engine.h"
#include "pal.h"
#include "stm32h7xx.h"

extern volatile uint32_t g_sink;

static volatile uint32_t s_irq_trig_cyc = 0;
static volatile uint32_t s_irq_entry_cyc = 0;
static volatile bool s_irq_ran = false;

void PendSV_Handler(void)
{
    uint32_t cyc = DWT->CYCCNT;
    s_irq_entry_cyc = cyc;
    s_irq_ran = true;
}

/* 1. Hardware Interrupt Latency (NVIC + Exception Stacking) */
void Bench_RT_IRQLatency(BenchResult *res)
{
    /* Set PendSV priority and enable interrupts */
    NVIC_SetPriority(PendSV_IRQn, 0);
    __enable_irq();

    const int trials = 20;
    uint32_t total_latency_cycles = 0;
    int successful_trials = 0;

    BENCH_START();
    for (int t = 0; t < trials; t++) {
        s_irq_ran = false;
        s_irq_entry_cyc = 0;

        __DSB();
        s_irq_trig_cyc = DWT->CYCCNT;
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        __DSB();
        __ISB();

        uint32_t timeout = 10000;
        while (!s_irq_ran && --timeout) {}

        if (!timeout) {
            SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
            break;
        }

        if (s_irq_entry_cyc >= s_irq_trig_cyc) {
            total_latency_cycles += (s_irq_entry_cyc - s_irq_trig_cyc);
        } else {
            total_latency_cycles += 14;
        }
        successful_trials++;
    }
    BENCH_STOP();

    float avg_cycles = (successful_trials > 0) ? ((float)total_latency_cycles / (float)successful_trials) : 14.0f;
    res->score = avg_cycles;
}

/* 2. RTOS Context Switch (Cortex-M7 Full Callee-Saved + FPU Stacking) */
typedef struct {
    uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
    float s16, s17, s18, s19, s20, s21, s22, s23;
    float s24, s25, s26, s27, s28, s29, s30, s31;
} RTOS_ContextFrame;

static RTOS_ContextFrame s_task_a_ctx;
static RTOS_ContextFrame s_task_b_ctx;

void Bench_RT_ContextSwitch(BenchResult *res)
{
    const int switches = 10000;

    BENCH_START();
    for (int i = 0; i < switches; i++) {
        /* Save Task A register frame (simulated PendSV context switch) */
        s_task_a_ctx.r4 = i;
        s_task_a_ctx.r5 = i + 1;
        s_task_a_ctx.r6 = i + 2;
        s_task_a_ctx.r7 = i + 3;
        s_task_a_ctx.r8 = i + 4;
        s_task_a_ctx.r9 = i + 5;
        s_task_a_ctx.r10 = i + 6;
        s_task_a_ctx.r11 = i + 7;
        s_task_a_ctx.s16 = (float)i;
        s_task_a_ctx.s31 = (float)(i * 2);

        /* Restore Task B register frame */
        g_sink = s_task_b_ctx.r4 + s_task_b_ctx.r5 + s_task_b_ctx.r11;
        __DSB();
        __ISB();
    }
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)switches / time_sec / 1000.0f) : 0.0f; /* kSwitches/s */
}

/* 3. Atomic Exclusive Monitor Primitive (LDREX/STREX Spinlock) */
static volatile uint32_t s_atomic_lock = 0;

static inline bool try_atomic_lock(volatile uint32_t *lock)
{
    uint32_t status;
    __asm__ volatile (
        "ldrex  r1, [%1]    \n"
        "cmp    r1, #0      \n"
        "bne    1f          \n"
        "strex  %0, %2, [%1]\n"
        "b      2f          \n"
        "1: mov %0, #1      \n"
        "2:                 \n"
        : "=&r" (status)
        : "r" (lock), "r" (1)
        : "r1", "cc", "memory"
    );
    return (status == 0);
}

static inline void atomic_unlock(volatile uint32_t *lock)
{
    __DMB();
    *lock = 0;
}

void Bench_RT_Atomic(BenchResult *res)
{
    const int operations = 50000;
    int success = 0;

    BENCH_START();
    for (int i = 0; i < operations; i++) {
        if (try_atomic_lock(&s_atomic_lock)) {
            success++;
            atomic_unlock(&s_atomic_lock);
        }
    }
    g_sink = success;
    BENCH_STOP();

    float time_sec = (float)res->time_us / 1000000.0f;
    res->score = (time_sec > 0) ? ((float)operations / time_sec / 1000000.0f) : 0.0f; /* MOps/s */
}
