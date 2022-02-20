set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Use newlib-nano from GNU Arm Embedded toolchain:
# execute_process(COMMAND arm-none-eabi-gcc -print-sysroot
#     OUTPUT_VARIABLE CMAKE_SYSROOT
#     OUTPUT_STRIP_TRAILING_WHITESPACE)

# Use custom newlib build for armv7em-none-eabi:
# - newlib-fseek-optimization     DISABLED
# - newlib-fvwrite-in-streamio    DISABLED
# - newlib-global-atexit           ENABLED
# - newlib-io-float               DISABLED
# - newlib-io-long-double         DISABLED
# - newlib-io-long-long            ENABLED
# - newlib-io-pos-args            DISABLED
# - newlib-io-c99-formats         DISABLED
# - newlib-multithread            DISABLED
# - newlib-nano-malloc             ENABLED
# - newlib-nano-formatted-io       ENABLED
# - newlib-reent-small             ENABLED
# - newlib-supplied-syscalls      DISABLED
# - newlib-unbuf-stream-opt       DISABLED
# - newlib-wide-orient            DISABLED
# - nls                           DISABLED
# - libgloss                      DISABLED
# - lite-exit                      ENABLED

set(CMAKE_SYSROOT /Applications/LLVM/arm-none-eabi)  

set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
set(CMAKE_ASM_COMPILER clang CACHE STRING "" FORCE)

set(CMAKE_CXX_COMPILER_TARGET armv7em-none-eabi)
set(CMAKE_C_COMPILER_TARGET armv7em-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET armv7em-none-eabi)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -mthumb
#     Generate ARMv7E-M Thumb instructions.
# -mcpu=cortex-m4
# -mfpu=none
#     Generate code for Arm Cortex-M4 without FPU.
# -mabi=aapcs
#     Use ARM Architecture Procedure Calling Standard (AAPCS) ABI.
# -mfloat-abi=soft
#     Use software library functions for floating-point operations.
# -ffunction-sections
# -fdata-sections
#     Place each function or data item into its own section in the
#     output file. Used with linker `--gc-sections` option.
# -ffreestanding
#     Asserts that program startup is not necessarily at `main`.
#     Implies `-fno-builtin`.
set(CMAKE_C_FLAGS_INIT
    "-mthumb -mcpu=cortex-m4 -mfpu=none -mabi=aapcs -mfloat-abi=soft -ffunction-sections -fdata-sections -ffreestanding")
set(CMAKE_CXX_FLAGS_INIT
    "-mthumb -mcpu=cortex-m4 -mfpu=none -mabi=aapcs -mfloat-abi=soft -ffunction-sections -fdata-sections -ffreestanding")

# -fuse-ld=lld
#     Use the LLVM linker.
# -rtlib=compiler-rt
#     Link with libclang_rt.builtins.
# --gcsections
#     Enable garbage collection of unused input sections. Used with
#     compiler `-fdata-sections` option.
# --omagic
#     Set the text and data sections to be readable and writable.
#     Also, do not page-align the data segment, and disable linking
#     against shared libraries.
# --sort-section=alignment
#     This option will apply "SORT_BY_ALIGNMENT" to all wildcard
#     section patterns in the linker script.
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-fuse-ld=lld -rtlib=compiler-rt -Wl,--gc-sections,--omagic,--sort-section=alignment")

# Build configuration flags for C.
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -Wall -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "" FORCE)

# Build configuration flags for C++.
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3 -Wall -DDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -Wall" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "" FORCE)

set(CMAKE_OBJCOPY llvm-objcopy CACHE INTERNAL "")
set(CMAKE_OBJDUMP llvm-objdump CACHE INTERNAL "")
set(CMAKE_SIZE llvm-size CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT} ${CMAKE_PREFIX_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
