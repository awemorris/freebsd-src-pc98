/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * The PC-98 kernel uses the current i386 entry path.  Keep the generated
 * assembly ABI tied to that implementation instead of carrying a second,
 * stale copy of the offset definitions.
 */

#include "../../i386/i386/genassym.c"
