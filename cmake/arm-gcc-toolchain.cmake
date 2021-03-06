set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

execute_process(COMMAND arm-none-eabi-gcc -print-sysroot
    OUTPUT_VARIABLE CMAKE_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_C_COMPILER arm-none-eabi-gcc CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++ CACHE STRING "" FORCE)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc CACHE STRING "" FORCE)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -mthumb
#     Generate ARMv7E-M Thumb instructions.
# -mcpu=cortex-m4+nofp
#     Generate code for Arm Cortex-M4 without FPU.
# -mfloat-abi=soft
#     Use software library functions for floating-point operations.
# -mabi=aapcs
#     Use ARM Architecture Procedure Calling Standard (AAPCS) ABI.
# --specs=nano.specs
#     Link with newlib-nano.
# -ffunction-sections
# -fdata-sections
#     Place each function or data item into its own section in the
#     output file. Used with linker `--gc-sections` option.
# -ffreestanding
#     Asserts that program startup is not necessarily at `main`.
#     Implies `-fno-builtin`.
# -fmerge-constants
#     Attempt to merge identical constants (string constants and
#     floating-point constants) across compilation units. 
# -fstack-usage
#     Generate an extra file that specifies the maximum amount of
#     stack used, on a per-function basis.
set(CMAKE_C_FLAGS_INIT
    "-mthumb -mcpu=cortex-m4+nofp -mfloat-abi=soft -mabi=aapcs --specs=nano.specs -ffunction-sections -fdata-sections -ffreestanding -fmerge-constants -fstack-usage")
set(CMAKE_CXX_FLAGS_INIT
    "-mthumb -mcpu=cortex-m4+nofp -mfloat-abi=soft -mabi=aapcs --specs=nano.specs -ffunction-sections -fdata-sections -ffreestanding -fmerge-constants -fstack-usage -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS_INIT
    "-x assembler-with-cpp -mthumb -mcpu=cortex-m4+nofp -mfloat-abi=soft")

# -nostartfiles
#     Do not use the standard system startup files when linking.
#     The standard system libraries are used normally, unless
#     `-nostdlib`, `-nolibc`, or `-nodefaultlibs` is used.
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
# --cref
#     Output a cross reference table.
# --print-memory-usage
#     Print used size, total size and used size of memory regions
#     created with the MEMORY command.
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-nostartfiles -lc_nano -Wl,--gc-sections,--omagic,--sort-section=alignment,--cref,--print-memory-usage")

# Build configuration flags for C.
set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -Wall" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "" FORCE)

# Build configuration flags for C++.
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -Wall" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall" CACHE STRING "" FORCE)

set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE INTERNAL "")
set(CMAKE_OBJDUMP arm-none-eabi-objdump CACHE INTERNAL "")
set(CMAKE_READELF arm-none-eabi-readelf CACHE INTERNAL "")
set(CMAKE_SIZE arm-none-eabi-size CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT} ${CMAKE_PREFIX_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

