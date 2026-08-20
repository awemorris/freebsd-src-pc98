/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/boot/pc98/libpc98/biosmem.c 298230 2016-04-18 23:09:22Z allanjude $");

/*
 * Obtain memory configuration information from the BIOS
 */
#include <stand.h>
#include <machine/cpufunc.h>
#include "libi386.h"
#include "btxv86.h"

vm_offset_t	memtop, memtop_copyin, high_heap_base;
uint32_t	bios_basemem, bios_extmem, high_heap_size;

#define	PC98_SYS16M_PORT	0x43b
#define	PC98_SYS16M_RAM_BIT	0x04
#define	PC98_LOW16_FULL_UNITS	120	/* (16MiB - 1MiB) / 128KiB */

uint8_t	pc98_sys16m_before, pc98_sys16m_after, pc98_low16_units;
int	pc98_sys16m_ram;

/*
 * The minimum amount of memory to reserve in bios_extmem for the heap.
 */
#define	HEAP_MIN	(64 * 1024 * 1024)

void
bios_getmem(void)
{
	uint16_t over16;

	/*
	 * Select RAM, rather than the PEGC/system-space window, at 15-16MiB.
	 * The port is read back and the BIOS work-area size must agree before
	 * the loader treats memory above 15MiB as one continuous range.
	 */
	pc98_sys16m_before = inb(PC98_SYS16M_PORT);
	outb(PC98_SYS16M_PORT,
	    pc98_sys16m_before | PC98_SYS16M_RAM_BIT);
	pc98_sys16m_after = inb(PC98_SYS16M_PORT);
	pc98_low16_units = *(volatile uint8_t *)PTOV(0xA1401);
	over16 = *(volatile uint16_t *)PTOV(0xA1594);
	pc98_sys16m_ram =
	    (pc98_sys16m_after & PC98_SYS16M_RAM_BIT) != 0 &&
	    (over16 == 0 || pc98_low16_units >= PC98_LOW16_FULL_UNITS);

    bios_basemem = ((*(u_char *)PTOV(0xA1501) & 0x07) + 1) * 128 * 1024;
    bios_extmem = pc98_low16_units * 128 * 1024;
    if (pc98_sys16m_ram)
	bios_extmem += over16 * 1024 * 1024;

    /* Set memtop to actual top of memory */
    memtop = memtop_copyin = 0x100000 + bios_extmem;

    /*
     * If we have extended memory, use the last 3MB of 'extended' memory
     * as a high heap candidate.
     */
    if (bios_extmem >= HEAP_MIN) {
	high_heap_size = HEAP_MIN;
	high_heap_base = memtop - HEAP_MIN;
    }
}    
