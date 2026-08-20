/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Minimal bridge between the PC-98 loader ABI and the current i386 init path.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/cpufunc.h>

#include <pc98/pc98/pc98_machdep.h>

CTASSERT(sizeof(uintptr_t) == 4);
CTASSERT(offsetof(struct bootinfo, bi_version) == 0);
CTASSERT(offsetof(struct bootinfo, bi_size) < sizeof(struct bootinfo));
CTASSERT(sizeof(bus_addr_t) == sizeof(uint32_t));
CTASSERT(sizeof(bus_space_handle_t) == sizeof(uintptr_t));

void
pc98_early_entry_checkpoint(u_int first)
{
	static const char marker[] = "PC98:P0:first-C\n";
	size_t i;

	/* Port 0xe9 is consumed only by the qemu bring-up debug console. */
	for (i = 0; i < nitems(marker) - 1; i++)
		outb(0xe9, marker[i]);
	/* Make the entry argument observable without depending on a console. */
	outb(0xe9, first != 0 ? '+' : '!');
	outb(0xe9, '\n');
}
