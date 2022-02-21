set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

execute_process(COMMAND clang -print-resource-dir
    OUTPUT_VARIABLE CMAKE_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

string(APPEND CMAKE_SYSROOT "/../../clang-runtimes/armv7em_soft_nofp")

set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
set(CMAKE_ASM_COMPILER clang CACHE STRING "" FORCE)

set(CMAKE_CXX_COMPILER_TARGET armv7em-none-eabi)
set(CMAKE_C_COMPILER_TARGET armv7em-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET armv7em-none-eabi)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -mthumb -march=armv7em
#     Generate ARMv7E-M Thumb instructions.
# -mcpu=cortex-m4 -mfpu=none
#     Generate code for Arm Cortex-M4 without FPU.
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
    "-mthumb -march=armv7em -mcpu=cortex-m4 -mfpu=none -mfloat-abi=soft -ffunction-sections -fdata-sections -ffreestanding")
set(CMAKE_CXX_FLAGS_INIT
    "-mthumb -march=armv7em -mcpu=cortex-m4 -mfpu=none -mfloat-abi=soft -ffunction-sections -fdata-sections -ffreestanding -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS_INIT
    "-x assembler-with-cpp -mthumb -march=armv7em -mcpu=cortex-m4 -mfloat-abi=soft")

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
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "" FORCE)

# Build configuration flags for C++.
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3 -Wall" CACHE STRING "" FORCE)
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
