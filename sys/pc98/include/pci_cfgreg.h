/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * PC-98 shares the i386 PCI configuration ABI, except for the machine's
 * non-standard configuration-mechanism-2 forward port.
 */

#ifndef _PC98_INCLUDE_PCI_CFGREG_H_
#define _PC98_INCLUDE_PCI_CFGREG_H_

#include <i386/pci_cfgreg.h>

#undef CONF2_FORWARD_PORT
#define CONF2_FORWARD_PORT 0x0cf9

#endif
