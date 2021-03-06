/** \file
 * Reboot into the hacked firmware.
 *
 * This program is very simple: attempt to reboot into the normal
 * firmware RAM image after startup.
 */
/*
 * Copyright (C) 2009 Trammell Hudson <hudson+ml@osresearch.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "compiler.h"
#include "consts.h"
#include "fw-signature.h"

#ifdef __ARM__
asm(
    ".text\n"
    ".globl _start\n"
    "_start:\n"

    /* if used in a .fir file, there is a 0x120 byte address offset.
       so cut the first 0x120 bytes off autoexec.bin before embedding into .fir
     */
    "B       skip_fir_header\n"
    ".incbin \"version.bin\"\n" // this must have exactly 0x11C (284) bytes
    "skip_fir_header:\n"

    "MRS     R0, CPSR\n"
    "BIC     R0, R0, #0x3F\n"   // Clear I,F,T
    "ORR     R0, R0, #0xD3\n"   // Set I,T, M=10011 == supervisor
    "MSR     CPSR, R0\n"
    "B       cstart\n"
);

/** Include the relocatable shim code */
extern uint8_t blob_start;
extern uint8_t blob_end;

asm(
    ".text\n"
    ".globl blob_start\n"
    "blob_start:\n"
    ".incbin \"magiclantern.bin\"\n"
    "blob_end:\n"
    ".globl blob_end\n"
);
#endif /* __ARM__ */

static void busy_wait(int n)
{
    int i,j;
    static volatile int k = 0;
    for (i = 0; i < n; i++)
        for (j = 0; j < 100000; j++)
            k++;
}

static void blink(int n)
{
    while (1)
    {
        #if defined(CARD_LED_ADDRESS) && defined(LEDON) && defined(LEDOFF)
        *(volatile int*) (CARD_LED_ADDRESS) = (LEDON);
        busy_wait(n);
        *(volatile int*)(CARD_LED_ADDRESS) = (LEDOFF);
        busy_wait(n);
        #endif
    }
}

static void fail()
{
    blink(50);
}

extern int compute_signature(int* start, int num);

void
__attribute__((noreturn))
cstart( void )
{

    #if !(CURRENT_CAMERA_SIGNATURE)
    #warning Signature Checking bypassed!! Please use a proper signature
    #else
    int s = compute_signature((int*)SIG_START, SIG_LEN);
    int _signature = (int)CURRENT_CAMERA_SIGNATURE;
    if (s != _signature)
        fail();
    #endif

#ifdef __ARM__
    /* turn on the LED as soon as autoexec.bin is loaded (may happen without powering on) */
    #if defined(CONFIG_40D) || defined(CONFIG_5DC)
        *(volatile int*) (LEDBLUE) = (LEDON);
        *(volatile int*) (LEDRED)  = (LEDON); // do we need the red too ?
    #elif defined(CARD_LED_ADDRESS) && defined(LEDON) // A more portable way, hopefully
        *(volatile int*) (CARD_LED_ADDRESS) = (LEDON);
    #endif

    blob_memcpy(
        (void*) RESTARTSTART,
        &blob_start,
        &blob_end
    );
    clean_d_cache();
    flush_caches();

    #if defined(CONFIG_7D)
        *(volatile int*)0xC0A00024 = 0x80000010; // send SSTAT for master processor, so it is in right state for rebooting
    #endif

    /* Jump into the newly relocated code
       Q: Why target/compiler-specific attribute long_call?
       A: If in any case the base address passed to linker (-Ttext 0x40800000) doesnt fit because we
          e.g. run at the cached address 0x00800000, we wont risk jumping into nirvana here.
          This will not help when the offset is oddly misplaced, like the 0x120 fir offset. Why?
          Because the code above (blob_memcpy) already made totally wrong assumptions about memory addresses.

       The name is not very inspired: it will not restart the camera, and may not copy anything.
       Keeping it for historical reasons (to match old docs).
       
       This function will patch Canon's startup code from main firmware in order to run ML, reserve memory
       for ML binary, starting from RESTARTSTART (where we already copied it), and... start it.
       
       Note: we can't just leave ML binary here (0x40800000), because the main firmware will reuse this area
       sooner or later. So, we have copied it to RESTARTSTART, and will tell Canon code not to touch it
       (usually by resizing some memory allocation pool and choosing RESTARTSTART in the newly created space).
    */
    void __attribute__((long_call)) (*copy_and_restart)() = (void*) RESTARTSTART;
    copy_and_restart();

#endif /* __ARM__ */

    // Unreachable
    while(1)
        ;
}
