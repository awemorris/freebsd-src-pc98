/*-
 * SPDX-License-Identifier: Beerware
 *
 * PC-98 P0 is uniprocessor, but common kernel code includes the machine SMP
 * declarations unconditionally.  The i386 header is empty unless SMP is set.
 */

#include <i386/smp.h>
