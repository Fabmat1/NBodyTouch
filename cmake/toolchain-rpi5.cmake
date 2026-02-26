# Cross-compilation toolchain: Arch Linux → Raspberry Pi 5 (aarch64)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Pi 5 = Cortex-A76, aarch64 is always hard-float (no -mfloat-abi needed)
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-a76")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-a76")