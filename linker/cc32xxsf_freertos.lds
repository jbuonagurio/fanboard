/*
 * Copyright (c) 2018-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

STACK_SIZE = 0x800; /* 2048 */

MEMORY
{
    FLASH_HDR (RX)  : ORIGIN = 0x01000000, LENGTH = 0x000007FF
    FLASH     (RX)  : ORIGIN = 0x01000800, LENGTH = 0x000FF800
    SRAM      (RWX) : ORIGIN = 0x20000000, LENGTH = 0x00040000
    /**
     * Explicitly placed off target for the storage of logging data.
     * The data placed here is NOT loaded onto the target device.
     * This is part of 1 GB of external memory from 0x60000000 - 0x9FFFFFFF.
     * ARM memory map can be found here:
     * https://developer.arm.com/documentation/ddi0337/e/memory-map/about-the-memory-map
     */
    LOG_DATA  (R)   : ORIGIN = 0x90000000, LENGTH = 0x00040000
}

REGION_ALIAS("REGION_HDR",       FLASH_HDR);
REGION_ALIAS("REGION_TEXT",      FLASH);
REGION_ALIAS("REGION_ARM_EXIDX", FLASH);
REGION_ALIAS("REGION_ARM_EXTAB", FLASH);
REGION_ALIAS("REGION_BSS",       SRAM);
REGION_ALIAS("REGION_DATA",      SRAM);
REGION_ALIAS("REGION_STACK",     SRAM);
REGION_ALIAS("REGION_HEAP",      SRAM);
REGION_ALIAS("REGION_LOG",       LOG_DATA);

SECTIONS
{
    .dbghdr : ALIGN(2048) {
        KEEP(*(.dbghdr))
    } > REGION_HDR

    PROVIDE(__reset_vectors_base = 0x01000800);

    .reset_vectors (__reset_vectors_base) : AT (__reset_vectors_base) {
        KEEP(*(.reset_vectors))
    } > REGION_TEXT

    .ram_vectors (NOLOAD) : ALIGN(1024) {
        KEEP(*(.ram_vectors))
    } > REGION_DATA

    .text : {
        CREATE_OBJECT_SYMBOLS
        *(.text)
        *(.text.*)
        . = ALIGN(4);
        KEEP(*(.ctors))
        . = ALIGN(4);
        KEEP(*(.dtors))
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(.init_array*))
        __init_array_end = .;
        *(.init)
        *(.fini*)
    } > REGION_TEXT AT> REGION_TEXT

    PROVIDE(__etext = .);

    .rodata : {
        *(.rodata)
        *(.rodata*)
    } > REGION_TEXT AT> REGION_TEXT

    .data : ALIGN(4) {
        __data_load__ = LOADADDR(.data);
        __data_start__ = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > REGION_DATA AT> REGION_TEXT
    
    PROVIDE_HIDDEN(__exidx_start = .);
    /DISCARD/ : {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    }
    PROVIDE_HIDDEN(__exidx_end = .);
    
    /* .ARM.exidx : {
        PROVIDE_HIDDEN(__exidx_start = .);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE_HIDDEN(__exidx_end = .);
    } > REGION_ARM_EXIDX AT> REGION_ARM_EXIDX */
    
    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > REGION_ARM_EXTAB AT> REGION_ARM_EXTAB

    .bss : {
        __bss_start__ = .;
        *(.shbss)
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > REGION_BSS AT> REGION_BSS

    .heap : {
        __heap_start__ = .;
        KEEP(*(.heap))
        __heap_end__ = .;
    } > REGION_HEAP AT> REGION_HEAP

    .stack (NOLOAD) : ALIGN(8) {
        __stack = .;
        KEEP(*(.stack))
        . += STACK_SIZE;
        __stack_end = .;
    } > REGION_STACK AT> REGION_STACK

    .log_data (COPY) : {
        KEEP(*(.log_data))
    } > REGION_LOG
}

ENTRY(Reset_Handler)
