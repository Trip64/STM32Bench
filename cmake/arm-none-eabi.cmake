# -------------------------------------------------------------------
# ARM Cortex-M cross-compilation toolchain file for CMake
# -------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Find the compiler - prefer STM32CubeCLT path, fall back to PATH
find_program(ARM_CC arm-none-eabi-gcc
    PATHS /opt/ST/STM32CubeCLT_*/GNU-tools-for-STM32/bin
    NO_DEFAULT_PATH
)
if(NOT ARM_CC)
    find_program(ARM_CC arm-none-eabi-gcc REQUIRED)
endif()
get_filename_component(ARM_TOOLCHAIN_DIR ${ARM_CC} DIRECTORY)

set(CMAKE_C_COMPILER   ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc)
set(CMAKE_OBJCOPY      ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP      ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objdump)
set(CMAKE_SIZE         ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-size)

# Prevent CMake from testing the compiler (it can't run ARM binaries on host)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search paths - only look in our toolchain, not the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
