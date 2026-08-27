cmake_minimum_required(VERSION 3.20)
project(stm32bench C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ===================================================================
# Board and Architecture Selection
# ===================================================================
set(BOARD "NUCLEO_H723ZG" CACHE STRING "Target Board: NUCLEO_H723ZG, NUCLEO_F072RB")
set(BENCH_SUITE "AUTO" CACHE STRING "Benchmark Suite: AUTO, BASIC, EXTENDED")
option(ENABLE_ETHERNET "Enable lwIP Ethernet HTTP server" ON)

if(BOARD STREQUAL "STM32F072_DISCO" OR BOARD STREQUAL "STM32F072B_DISCO" OR BOARD STREQUAL "DISCO_F072RB")
    message(STATUS "Configuring for Board: STM32F072B-DISCO (Discovery Kit, ARM Cortex-M0 @ 48 MHz)")
    set(MCU_FLAGS
        -mcpu=cortex-m0
        -mthumb
        -mfloat-abi=soft
    )
    add_compile_definitions(
        STM32F072xB
        ARM_MATH_CM0
        BOARD_STM32F072_DISCO
    )
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F072RBTx.ld")
    set(BOARD_SOURCES
        src/startup_stm32f072xb.s
        src/pal/pal_stm32f0.c
        src/bsp/bsp_disco_f072bd.c
    )
    set(ENABLE_ETHERNET OFF)
    if(BENCH_SUITE STREQUAL "AUTO" OR BENCH_SUITE STREQUAL "BASIC")
        set(SUITE_EXTENDED 0)
    else()
        set(SUITE_EXTENDED 1)
    endif()

elseif(BOARD STREQUAL "NUCLEO_F072RB")
    message(STATUS "Configuring for Board: NUCLEO-F072RB (ARM Cortex-M0 @ 48 MHz)")
    set(MCU_FLAGS
        -mcpu=cortex-m0
        -mthumb
        -mfloat-abi=soft
    )
    add_compile_definitions(
        STM32F072xB
        ARM_MATH_CM0
        BOARD_NUCLEO_F072RB
    )
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F072RBTx.ld")
    set(BOARD_SOURCES
        src/startup_stm32f072xb.s
        src/pal/pal_stm32f0.c
        src/bsp/bsp_nucleo_f072rb.c
    )
    # F072 has no Ethernet hardware; default to BASIC suite
    set(ENABLE_ETHERNET OFF)
    if(BENCH_SUITE STREQUAL "AUTO" OR BENCH_SUITE STREQUAL "BASIC")
        set(SUITE_EXTENDED 0)
    else()
        set(SUITE_EXTENDED 1)
    endif()

else() # Default: NUCLEO_H723ZG
    message(STATUS "Configuring for Board: NUCLEO-H723ZG (ARM Cortex-M7 @ 550 MHz)")
    set(MCU_FLAGS
        -mcpu=cortex-m7
        -mthumb
        -mfpu=fpv5-d16
        -mfloat-abi=hard
    )
    add_compile_definitions(
        STM32H723xx
        ARM_MATH_CM7
        USE_FULL_LL_DRIVER
        BOARD_NUCLEO_H723ZG
    )
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32H723ZGTx.ld")
    set(BOARD_SOURCES
        src/startup_stm32h723xx.s
        src/system_stm32h7xx.c
        src/pal/pal_stm32h7.c
        src/bsp/bsp_nucleo_h723zg.c
    )
    if(BENCH_SUITE STREQUAL "BASIC")
        set(SUITE_EXTENDED 0)
    else()
        set(SUITE_EXTENDED 1)
    endif()
endif()

# Suite definitions
if(SUITE_EXTENDED)
    message(STATUS "Benchmark Suite: EXTENDED (Full 48+ Hardware & Accelerator Tests)")
    add_compile_definitions(BENCH_SUITE_EXTENDED=1)
else()
    message(STATUS "Benchmark Suite: BASIC (14 Core Universal Tests, <6KB RAM)")
    add_compile_definitions(BENCH_SUITE_EXTENDED=0)
endif()

# Ethernet configuration
if(ENABLE_ETHERNET)
    add_compile_definitions(ENABLE_ETHERNET=1)
else()
    add_compile_definitions(ENABLE_ETHERNET=0)
endif()

# Transport configuration: UART & Native USB User Port
option(ENABLE_UART "Enable UART transport" ON)
option(ENABLE_USB_USER "Enable Native On-Chip USB User Port" ON)

if(ENABLE_UART)
    add_compile_definitions(ENABLE_UART=1)
else()
    add_compile_definitions(ENABLE_UART=0)
endif()

if(ENABLE_USB_USER AND (BOARD MATCHES "F072"))
    add_compile_definitions(ENABLE_USB_USER=1)
    message(STATUS "Transport: Native On-Chip USB CDC ENABLED (PA11/PA12 User USB)")
else()
    add_compile_definitions(ENABLE_USB_USER=0)
endif()

# ===================================================================
# Compiler & Linker Flags
# ===================================================================
add_compile_options(
    ${MCU_FLAGS}
    -O3
    -g3
    -Wall
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
    -fno-common
)

add_link_options(
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--no-warn-rwx-segments
    --specs=nano.specs
    -u _printf_float
    -lc
    -lm
)

# Base Include Directories
set(INC_DIRS
    src
    src/pal
    src/bsp
    src/bench
    src/output
    src/usb
    lib/cmsis/include
    lib/cmsis/device
)

if(ENABLE_ETHERNET)
    list(APPEND INC_DIRS
        src/net
        lib/lwip/src/include
    )
endif()

include_directories(${INC_DIRS})

# ===================================================================
# Core Source Files
# ===================================================================
set(APP_SOURCES
    ${BOARD_SOURCES}
    src/syscalls.c
    src/main.c
    src/bench/bench_engine.c
    src/bench/bench_cpu.c
    src/bench/bench_fpu.c
    src/bench/bench_dsp.c
    src/bench/bench_io.c
    src/bench/bench_memory.c
    src/output/json_output.c
    src/output/uart_output.c
    src/usb/usb_cdc.c
)

if(ENABLE_USB_USER AND (BOARD MATCHES "F072"))
    list(APPEND APP_SOURCES src/usb/usb_f0_fs.c)
endif()

# Extended Suite Benchmarks
if(SUITE_EXTENDED)
    list(APPEND APP_SOURCES
        src/bench/bench_cordic.c
        src/bench/bench_fmac.c
        src/bench/bench_gfx.c
        src/bench/bench_tinyml.c
        src/bench/bench_crypto.c
        src/bench/bench_compress.c
        src/bench/bench_audio.c
        src/bench/bench_realtime.c
    )
    if(BOARD STREQUAL "NUCLEO_H723ZG")
        list(APPEND APP_SOURCES src/bench/bench_h7_features.c)
    endif()
endif()

# Optional Ethernet & lwIP Sources
if(ENABLE_ETHERNET)
    list(APPEND APP_SOURCES
        src/net/ethernetif.c
        src/net/httpd_handlers.c
        src/net/net_init.c
        src/net/fsdata.c
        lib/lwip/src/core/init.c
        lib/lwip/src/core/def.c
        lib/lwip/src/core/mem.c
        lib/lwip/src/core/memp.c
        lib/lwip/src/core/pbuf.c
        lib/lwip/src/core/netif.c
        lib/lwip/src/core/ip.c
        lib/lwip/src/core/timeouts.c
        lib/lwip/src/core/inet_chksum.c
        lib/lwip/src/core/tcp.c
        lib/lwip/src/core/tcp_in.c
        lib/lwip/src/core/tcp_out.c
        lib/lwip/src/core/ipv4/etharp.c
        lib/lwip/src/core/ipv4/ip4.c
        lib/lwip/src/core/ipv4/ip4_addr.c
        lib/lwip/src/core/ipv4/ip4_frag.c
        lib/lwip/src/core/ipv4/icmp.c
        lib/lwip/src/netif/ethernet.c
        lib/lwip/src/apps/http/httpd.c
        lib/lwip/src/apps/http/fs.c
    )
endif()

# ===================================================================
# Executable & Outputs
# ===================================================================
add_executable(${PROJECT_NAME}.elf ${APP_SOURCES})

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${PROJECT_NAME}.bin and printing memory usage..."
)
