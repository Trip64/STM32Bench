# ===================================================================
# STM32 Benchmark Suite - Makefile wrapper around CMake
# ===================================================================

BUILD_DIR   ?= build
BUILD_TYPE  ?= Release
CMAKE_EXTRA ?=

# Default target board and features
BOARD           ?= NUCLEO_H723ZG
BENCH_SUITE     ?= AUTO
ENABLE_ETHERNET ?= 1
ENABLE_UART     ?= 1
ENABLE_USB_USER ?= 1

.PHONY: all configure build flash clean rebuild deps fsdata usb-dashboard

all: build

# ---- USB Web Dashboard -------------------------------------------
usb-dashboard:
	@echo "==> Opening USB Web Serial Dashboard in browser..."
	@open tools/usb_dashboard.html 2>/dev/null || xdg-open tools/usb_dashboard.html 2>/dev/null || echo "Open tools/usb_dashboard.html in Chrome/Edge/Brave"

# ---- Dependencies ------------------------------------------------
deps:
	@echo "==> Fetching dependencies (CMSIS + lwIP)..."
	bash tools/fetch_deps.sh

# ---- Generate embedded filesystem --------------------------------
fsdata:
	@echo "==> Generating fsdata.c from web assets..."
	python3 tools/makefsdata.py src/net/web src/net/fsdata.c

# ---- CMake configure + build ------------------------------------
configure:
	cmake -B $(BUILD_DIR) \
	      -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
	      -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	      -DBOARD=$(BOARD) \
	      -DBENCH_SUITE=$(BENCH_SUITE) \
	      -DENABLE_ETHERNET=$(ENABLE_ETHERNET) \
	      -DENABLE_UART=$(ENABLE_UART) \
	      -DENABLE_USB_USER=$(ENABLE_USB_USER) \
	      $(CMAKE_EXTRA)

build: configure
	cmake --build $(BUILD_DIR) -- -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu)
	@echo ""
	@echo "==> Build complete.  Output:"
	@ls -lh $(BUILD_DIR)/stm32bench.elf $(BUILD_DIR)/stm32bench.bin 2>/dev/null || true

# Detect STM32_Programmer_CLI
STM32_PROG := $(shell which STM32_Programmer_CLI 2>/dev/null || ls /opt/ST/STM32CubeCLT_*/STM32CubeProgrammer/bin/STM32_Programmer_CLI 2>/dev/null | head -1)

# ---- Flash -------------------------------------------------------
flash: build
	@echo "==> Flashing via STM32CubeProgrammer ($(STM32_PROG))..."
	$(STM32_PROG) -c port=SWD -w $(BUILD_DIR)/stm32bench.bin 0x08000000 -v -hardRst -run

flash-openocd: build
	@echo "==> Flashing via OpenOCD..."
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	        -c "program $(BUILD_DIR)/stm32bench.elf verify reset exit"

# ---- Clean -------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)

rebuild: clean build
