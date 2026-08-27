#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

CMSIS_DIR="${ROOT_DIR}/lib/cmsis"
LWIP_DIR="${ROOT_DIR}/lib/lwip"

echo "=== Fetching STM32H7 CMSIS & ARM CMSIS Core Headers ==="
mkdir -p "${CMSIS_DIR}/include"
mkdir -p "${CMSIS_DIR}/device"

CMSIS_CORE_BASE="https://raw.githubusercontent.com/ARM-software/CMSIS_5/5.9.0/CMSIS/Core/Include"
CMSIS_DEV_H7_BASE="https://raw.githubusercontent.com/STMicroelectronics/cmsis_device_h7/master/Include"
CMSIS_DEV_F0_BASE="https://raw.githubusercontent.com/STMicroelectronics/cmsis_device_f0/master/Include"

download_if_missing() {
    local url="$1"
    local dest="$2"
    if [ ! -f "$dest" ]; then
        echo "  Downloading $(basename "$dest")..."
        curl -s -f -L "$url" -o "$dest"
    else
        echo "  Already present: $(basename "$dest")"
    fi
}

download_if_missing "${CMSIS_CORE_BASE}/core_cm7.h" "${CMSIS_DIR}/include/core_cm7.h"
download_if_missing "${CMSIS_CORE_BASE}/core_cm0.h" "${CMSIS_DIR}/include/core_cm0.h"
download_if_missing "${CMSIS_CORE_BASE}/cmsis_version.h" "${CMSIS_DIR}/include/cmsis_version.h"
download_if_missing "${CMSIS_CORE_BASE}/cmsis_compiler.h" "${CMSIS_DIR}/include/cmsis_compiler.h"
download_if_missing "${CMSIS_CORE_BASE}/cmsis_gcc.h" "${CMSIS_DIR}/include/cmsis_gcc.h"
download_if_missing "${CMSIS_CORE_BASE}/mpu_armv7.h" "${CMSIS_DIR}/include/mpu_armv7.h"
download_if_missing "${CMSIS_CORE_BASE}/cachel1_armv7.h" "${CMSIS_DIR}/include/cachel1_armv7.h"

download_if_missing "${CMSIS_DEV_H7_BASE}/stm32h7xx.h" "${CMSIS_DIR}/device/stm32h7xx.h"
download_if_missing "${CMSIS_DEV_H7_BASE}/stm32h723xx.h" "${CMSIS_DIR}/device/stm32h723xx.h"
download_if_missing "${CMSIS_DEV_H7_BASE}/system_stm32h7xx.h" "${CMSIS_DIR}/device/system_stm32h7xx.h"

download_if_missing "${CMSIS_DEV_F0_BASE}/stm32f0xx.h" "${CMSIS_DIR}/device/stm32f0xx.h"
download_if_missing "${CMSIS_DEV_F0_BASE}/stm32f072xb.h" "${CMSIS_DIR}/device/stm32f072xb.h"
download_if_missing "${CMSIS_DEV_F0_BASE}/system_stm32f0xx.h" "${CMSIS_DIR}/device/system_stm32f0xx.h"

echo "=== Fetching lwIP (Lightweight IP stack) ==="
if [ ! -d "${LWIP_DIR}/src" ]; then
    echo "  Cloning lwIP git repo (shallow tag STABLE-2_2_0_RELEASE)..."
    git clone --depth 1 --branch STABLE-2_2_0_RELEASE https://github.com/lwip-tcpip/lwip.git "${LWIP_DIR}"
else
    echo "  lwIP already present at ${LWIP_DIR}"
fi

echo "=== Dependencies ready ==="
