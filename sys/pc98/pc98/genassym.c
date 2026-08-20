/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * The PC-98 kernel uses the current i386 entry path.  Keep the generated
 * assembly ABI tied to that implementation instead of carrying a second,
 * stale copy of the offset definitions.
 */

#include "../../i386/i386/genassym.c"

/* Private offsets consumed only by pc98/pc98/busio.s. */
#ifdef PC98
#include <machine/bus.h>
ASSYM(BUS_SPACE_HANDLE_BASE, offsetof(struct bus_space_handle, bsh_base));
ASSYM(BUS_SPACE_HANDLE_IAT, offsetof(struct bus_space_handle, bsh_iat));
#endif
