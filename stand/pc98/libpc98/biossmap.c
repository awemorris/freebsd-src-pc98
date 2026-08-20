/*-
 * Copyright (c) 2006 TAKAHASHI Yoshihiro <nyan@FreeBSD.org>
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
__FBSDID("$FreeBSD: head/sys/boot/pc98/libpc98/biossmap.c 164115 2006-11-09 08:28:02Z nyan $");

#include <stand.h>
#include <sys/param.h>
#include <sys/linker.h>
#include <machine/metadata.h>
#include <i386/pc/bios.h>
#include "bootstrap.h"
#include "libi386.h"

static struct bios_smap pc98_smap[3];
static u_int pc98_smaplen;

static int
pc98_smap_valid(void)
{
	uint64_t end, previous_end;
	u_int i;

	previous_end = 0;
	for (i = 0; i < pc98_smaplen; i++) {
		if (pc98_smap[i].length == 0 ||
		    pc98_smap[i].base > UINT64_MAX - pc98_smap[i].length)
			return (0);
		end = pc98_smap[i].base + pc98_smap[i].length;
		if (i != 0 && pc98_smap[i].base < previous_end)
			return (0);
		previous_end = end;
	}
	return (pc98_smaplen != 0);
}

void
bios_getsmap(void)
{
	uint64_t top;

	pc98_smaplen = 0;
	top = memtop;
	if (bios_basemem != 0) {
		pc98_smap[pc98_smaplen].base = 0;
		pc98_smap[pc98_smaplen].length = bios_basemem;
		pc98_smap[pc98_smaplen].type = SMAP_TYPE_MEMORY;
		pc98_smaplen++;
	}
	if (bios_basemem < 0x100000) {
		pc98_smap[pc98_smaplen].base = bios_basemem;
		pc98_smap[pc98_smaplen].length = 0x100000 - bios_basemem;
		pc98_smap[pc98_smaplen].type = SMAP_TYPE_RESERVED;
		pc98_smaplen++;
	}
	if (top > 0x100000) {
		pc98_smap[pc98_smaplen].base = 0x100000;
		pc98_smap[pc98_smaplen].length = top - 0x100000;
		pc98_smap[pc98_smaplen].type = SMAP_TYPE_MEMORY;
		pc98_smaplen++;
	}
	if (!pc98_smap_valid())
		pc98_smaplen = 0;
}

void
bios_addsmapdata(struct preloaded_file *kfp)
{
	size_t size;

	if (!pc98_smap_valid())
		return;
	size = pc98_smaplen * sizeof(pc98_smap[0]);
	file_addmetadata(kfp, MODINFOMD_SMAP, size, pc98_smap);
}

COMMAND_SET(smap, "smap", "show PC-98 memory map", command_smap);

static int
command_smap(int argc, char *argv[])
{
	u_int i;

	if (!pc98_smap_valid())
		return (CMD_ERROR);
	for (i = 0; i < pc98_smaplen; i++)
		printf("SMAP type=%02x base=%016llx len=%016llx\n",
		    (unsigned int)pc98_smap[i].type,
		    (unsigned long long)pc98_smap[i].base,
		    (unsigned long long)pc98_smap[i].length);
	return (CMD_OK);
}
