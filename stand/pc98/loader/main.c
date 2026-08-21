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
__FBSDID("$FreeBSD: head/sys/boot/pc98/loader/main.c 310225 2016-12-18 13:57:23Z emaste $");

/*
 * MD bootstrap main() and assorted miscellaneous
 * commands.
 */

#include <stand.h>
#include <stddef.h>
#include <string.h>
#include <machine/bootinfo.h>
#include <machine/cpufunc.h>
#include <sys/param.h>
#include <sys/reboot.h>

#include "bootstrap.h"
#include "common/bootargs.h"
#include "libi386/libi386.h"
#include "libpc98/libpc98.h"
#include "btxv86.h"

_Static_assert(sizeof(struct bootargs) == BOOTARGS_SIZE, "Bootarg size bad");
_Static_assert(offsetof(struct bootargs, bootinfo) == BA_BOOTINFO,
    "BA_BOOTINFO");
_Static_assert(offsetof(struct bootargs, bootflags) == BA_BOOTFLAGS,
    "BA_BOOTFLAGS");
_Static_assert(offsetof(struct bootinfo, bi_size) == BI_SIZE, "BI_SIZE");

/* Arguments passed in from the boot1/boot2 loader */
static struct bootargs *kargs;

static u_int32_t	initial_howto;
static u_int32_t	initial_bootdev;
static struct bootinfo	*initial_bootinfo;

struct arch_switch	archsw;		/* MI/MD interface boundary */

static void		extract_currdev(void);
static int		isa_inb(int port);
static void		isa_outb(int port, int value);
void			exit(int code);

/* from vers.c */
extern	char bootprog_info[];

/* XXX debugging */
extern char end[];

static void *heap_top;
static void *heap_bottom;

int
main(void)
{
    int			i;

    /* Set machine type to PC98_SYSTEM_PARAMETER. */
    set_machine_type();

    /* Pick up arguments */
    kargs = (void *)__args;
    initial_howto = kargs->howto;
    initial_bootdev = kargs->bootdev;
    initial_bootinfo = kargs->bootinfo ? (struct bootinfo *)PTOV(kargs->bootinfo) : NULL;

    /* Initialize the v86 register set to a known-good state. */
    bzero(&v86, sizeof(v86));
    v86.efl = PSL_RESERVED_DEFAULT | PSL_I;

    /* 
     * Initialise the heap as early as possible.  Once this is done, malloc() is usable.
     */
    bios_getmem();

#if defined(LOADER_BZIP2_SUPPORT) || defined(LOADER_GZIP_SUPPORT) || \
    defined(LOADER_GPT_SUPPORT) || defined(LOADER_ZFS_SUPPORT)
    if (high_heap_size > 0) {
	heap_top = PTOV(high_heap_base + high_heap_size);
	heap_bottom = PTOV(high_heap_base);
	if (high_heap_base < memtop_copyin)
	    memtop_copyin = high_heap_base;
    } else
#endif
    {
	heap_top = (void *)PTOV(bios_basemem);
	heap_bottom = (void *)end;
    }
    setheap(heap_bottom, heap_top);

    /* 
     * XXX Chicken-and-egg problem; we want to have console output early, but some
     * console attributes may depend on reading from eg. the boot device, which we
     * can't do yet.
     *
     * We can use printf() etc. once this is done.
     * If the previous boot stage has requested a serial console, prefer that.
     */
    bi_setboothowto(initial_howto);
    if (initial_howto & RB_MULTIPLE) {
	if (initial_howto & RB_SERIAL)
	    setenv("console", "comconsole vidconsole", 1);
	else
	    setenv("console", "vidconsole comconsole", 1);
    } else if (initial_howto & RB_SERIAL)
	setenv("console", "comconsole", 1);
    else if (initial_howto & RB_MUTE)
	setenv("console", "nullconsole", 1);
    cons_probe();

    printf("PC-98 15-16MiB memory: %s "
	"(port 0x43b %#x->%#x, low16=%uKiB)\n",
	pc98_sys16m_ram ? "RAM; PEGC window disabled" :
	"not trusted; memory capped below the window",
	pc98_sys16m_before, pc98_sys16m_after,
	(unsigned int)pc98_low16_units * 128 + 1024);

    /*
     * Initialise the block cache. Set the upper limit.
     */
    bcache_init(32768, 512);

    if (kargs->bootinfo == 0 &&
	(kargs->bootflags & KARGS_FLAGS_CD) != 0)
	bc_add(initial_bootdev);

    archsw.arch_autoload = i386_autoload;
    archsw.arch_getdev = i386_getdev;
    archsw.arch_copyin = i386_copyin;
    archsw.arch_copyout = i386_copyout;
    archsw.arch_readin = i386_readin;
    archsw.arch_isainb = isa_inb;
    archsw.arch_isaoutb = isa_outb;
    /*
     * March through the device switch probing for things.
     */
    for (i = 0; devsw[i] != NULL; i++)
	if (devsw[i]->dv_init != NULL)
	    (devsw[i]->dv_init)();
    printf("BIOS %dkB/%dkB available memory\n", bios_basemem / 1024, bios_extmem / 1024);
    if (initial_bootinfo != NULL) {
	initial_bootinfo->bi_basemem = bios_basemem / 1024;
	initial_bootinfo->bi_extmem = bios_extmem / 1024;
    }

    printf("\n%s", bootprog_info);

    extract_currdev();				/* set $currdev and $loaddev */
    setenv("LINES", "24", 1);			/* optional */
	/* The minimal recovery loader enters its prompt unless a script boots. */
	setenv("autoboot_delay", "NO", 1);

    bios_getsmap();

    interact();				/* doesn't return */

    /* if we ever get here, it is an error */
    return (1);
}

/*
 * Set the 'current device' by (if possible) recovering the boot device as 
 * supplied by the initial bootstrap.
 *
 * XXX should be extended for netbooting.
 */
static void
extract_currdev(void)
{
    struct i386_devdesc		new_currdev;
    int				major;
    int				biosdev = -1;

    bzero(&new_currdev, sizeof(new_currdev));

    /* Assume a BIOS disk unless cdboot explicitly identifies a CD. */
    new_currdev.dd.d_dev = &bioshd;

    /* new-style boot loaders such as pxeldr and cdldr */
    if (kargs->bootinfo == 0) {
	if ((kargs->bootflags & KARGS_FLAGS_CD) != 0) {
	    new_currdev.dd.d_dev = &bioscd;
	    new_currdev.dd.d_unit = bc_bios2unit(initial_bootdev);
	} else {
	    /* We don't know what our boot device is; use the disk fallback. */
	    new_currdev.disk.d_slice = D_SLICENONE;
	    new_currdev.disk.d_partition = 0;
	    biosdev = -1;
	}
    } else if ((initial_bootdev & B_MAGICMASK) != B_DEVMAGIC) {
	/* The passed-in boot device is bad */
	new_currdev.disk.d_slice = D_SLICENONE;
	new_currdev.disk.d_partition = 0;
	biosdev = -1;
    } else {
	new_currdev.disk.d_slice = B_SLICE(initial_bootdev) - 1;
	new_currdev.disk.d_partition = B_PARTITION(initial_bootdev);
	biosdev = initial_bootinfo->bi_bios_dev;
	major = B_TYPE(initial_bootdev);

	/*
	 * If we are booted by an old bootstrap, we have to guess at the BIOS
	 * unit number.  We will lose if there is more than one disk type
	 * and we are not booting from the lowest-numbered disk type 
	 * (ie. SCSI when IDE also exists).
	 */
	if ((biosdev == 0) && (B_TYPE(initial_bootdev) != 2)) {	/* biosdev doesn't match major */
	    if (B_TYPE(initial_bootdev) == 6)
		biosdev = 0x30 + B_UNIT(initial_bootdev);
	    else
		biosdev = (major << 3) + 0x80 + B_UNIT(initial_bootdev);
	}
    }
    /*
     * If we are booting off of a BIOS disk and we didn't succeed in determining
     * which one we booted off of, just use disk0: as a reasonable default.
     */
    if ((new_currdev.dd.d_dev->dv_type == bioshd.dv_type) &&
	((new_currdev.dd.d_unit = bd_bios2unit(biosdev)) == -1)) {
	printf("Can't work out which disk we are booting from.\n"
	       "Guessed BIOS device 0x%x not found by probes, defaulting to disk0:\n", biosdev);
	new_currdev.dd.d_unit = 0;
    }

    env_setenv("currdev", EV_VOLATILE, devformat(&new_currdev.dd),
	       gen_setcurrdev, env_nounset);
    env_setenv("loaddev", EV_VOLATILE, devformat(&new_currdev.dd), env_noset,
	       env_nounset);
}

COMMAND_SET(reboot, "reboot", "reboot the system", command_reboot);

static int
command_reboot(int argc, char *argv[])
{
    int i;

    for (i = 0; devsw[i] != NULL; ++i)
	if (devsw[i]->dv_cleanup != NULL)
	    (devsw[i]->dv_cleanup)();

    printf("Rebooting...\n");
    delay(1000000);
    __exit(0);
}

/* provide this for panic, as it's not in the startup code */
void
exit(int code)
{
    __exit(code);
}

COMMAND_SET(heap, "heap", "show heap usage", command_heap);

static int
command_heap(int argc, char *argv[])
{
    mallocstats();
    printf("heap base at %p, top at %p, upper limit at %p\n", heap_bottom,
      sbrk(0), heap_top);
    return(CMD_OK);
}

/* ISA bus access functions for PnP. */
static int
isa_inb(int port)
{

    return (inb(port));
}

static void
isa_outb(int port, int value)
{

    outb(port, value);
}
