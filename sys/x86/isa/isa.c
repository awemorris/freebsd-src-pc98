/*-
 * SPDX-License-Identifier: (BSD-2-Clause AND ISC)
 *
 * Copyright (c) 1998 Doug Rabson
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
/*-
 * Modifications for Intel architecture by Garrett A. Wollman.
 * Copyright 1998 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <machine/bus.h>
#include <sys/rman.h>

#include <machine/resource.h>

#include <isa/isavar.h>
#include <isa/isa_common.h>

void
isa_init(device_t dev)
{
}

/*
 * This implementation simply passes the request up to the parent
 * bus, which in our case is the special i386 nexus, substituting any
 * configured values if the caller defaulted.  We can get away with
 * this because there is no special mapping for ISA resources on an Intel
 * platform.  When porting this code to another architecture, it may be
 * necessary to interpose a mapping layer here.
 */
struct resource *
isa_alloc_resource(device_t bus, device_t child, int type, int *rid,
		   rman_res_t start, rman_res_t end, rman_res_t count, u_int flags)
{
	/*
	 * Consider adding a resource definition.
	 */
	int passthrough = (device_get_parent(child) != bus);
	int isdefault = RMAN_IS_DEFAULT_RANGE(start, end);
	struct isa_device* idev = DEVTOISA(child);
	struct resource_list *rl = &idev->id_resources;
	struct resource_list_entry *rle;

	if (!passthrough && !isdefault) {
		rle = resource_list_find(rl, type, *rid);
		if (!rle) {
			if (*rid < 0)
				return 0;
			switch (type) {
			case SYS_RES_IRQ:
				if (*rid >= ISA_NIRQ)
					return 0;
				break;
			case SYS_RES_DRQ:
				if (*rid >= ISA_NDRQ)
					return 0;
				break;
			case SYS_RES_MEMORY:
				if (*rid >= ISA_NMEM)
					return 0;
				break;
			case SYS_RES_IOPORT:
				if (*rid >= ISA_NPORT)
					return 0;
				break;
			default:
				return 0;
			}
			resource_list_add(rl, type, *rid, start, end, count);
		}
	}

	return resource_list_alloc(rl, bus, child, type, rid,
				   start, end, count, flags);
}

#ifdef PC98
/*
 * Reserve a sparse PC-98 I/O vector as independent contiguous extents.
 * The primary handle owns the array of secondary resources.  Allocation is
 * all-or-nothing and every failure path releases in strict reverse order.
 */
struct resource *
isa_alloc_resourcev(device_t child, int type, int *rid, bus_addr_t *res,
    bus_size_t count, u_int flags)
{
	struct isa_device *idev;
	struct resource_list *rl;
	struct resource **resources;
	struct resource *primary;
	bus_space_handle_t bh;
	device_t bus;
	rman_res_t base;
	bus_size_t i, j, nsegments;
	int allocated, bsrid;

	if (child == NULL || rid == NULL || res == NULL || *rid < 0 ||
	    count == 0 || count > BUS_SPACE_IAT_MAXSIZE)
		return (NULL);
	if (type != SYS_RES_IOPORT && type != SYS_RES_MEMORY)
		return (NULL);

	for (i = 1; i < count; i++)
		if (res[i] <= res[i - 1])
			return (NULL);

	base = bus_get_resource_start(child, type, *rid);
	if (base > BUS_SPACE_MAXADDR)
		return (NULL);
	for (i = 0; i < count; i++)
		if (res[i] > BUS_SPACE_MAXADDR - base)
			return (NULL);

	nsegments = 1;
	for (i = 1; i < count; i++)
		if (res[i] != res[i - 1] + 1)
			nsegments++;

	resources = malloc(sizeof(*resources) * nsegments, M_DEVBUF,
	    M_NOWAIT | M_ZERO);
	if (resources == NULL)
		return (NULL);

	idev = DEVTOISA(child);
	rl = &idev->id_resources;
	bus = device_get_parent(child);
	allocated = 0;
	for (i = 0; i < count; i = j) {
		for (j = i + 1; j < count && res[j] == res[j - 1] + 1; j++)
			;
		bsrid = *rid + allocated;
		resources[allocated] = isa_alloc_resource(bus, child, type,
		    &bsrid, base + res[i], base + res[j - 1], j - i, flags);
		if (resources[allocated] == NULL)
			goto fail;
		allocated++;
	}

	primary = resources[0];
	bh = rman_get_bushandle(primary);
	if (bh == NULL)
		goto fail;
	bh->bsh_res = resources;
	bh->bsh_ressz = nsegments;
	return (primary);

fail:
	while (allocated != 0) {
		allocated--;
		(void)resource_list_release(rl, bus, child,
		    resources[allocated]);
	}
	free(resources, M_DEVBUF);
	return (NULL);
}

int
isa_load_resourcev(struct resource *r, bus_addr_t *res, bus_size_t count)
{

	if (r == NULL)
		return (EINVAL);
	return (bus_space_map_load(rman_get_bustag(r), rman_get_bushandle(r),
	    count, res, 0));
}
#endif

int
isa_release_resource(device_t bus, device_t child, struct resource *r)
{
	struct isa_device* idev = DEVTOISA(child);
	struct resource_list *rl = &idev->id_resources;

#ifdef PC98
	struct resource **resources;
	bus_space_handle_t bh;
	size_t count;
	int error, i, primary_error, type;

	error = 0;
	type = rman_get_type(r);
	if (type == SYS_RES_MEMORY || type == SYS_RES_IOPORT) {
		bh = rman_get_bushandle(r);
		if (bh != NULL && bh->bsh_res != NULL) {
			resources = bh->bsh_res;
			count = bh->bsh_ressz;
			bh->bsh_res = NULL;
			bh->bsh_ressz = 0;
			for (i = (int)count - 1; i > 0; i--)
				if (resource_list_release(rl, bus, child,
				    resources[i]) != 0)
					error = EBUSY;
			free(resources, M_DEVBUF);
		}
	}
	primary_error = resource_list_release(rl, bus, child, r);
	return (error != 0 ? error : primary_error);
#else
	return (resource_list_release(rl, bus, child, r));
#endif
}

/*
 * On this platform, isa can also attach to the legacy bus.
 */
DRIVER_MODULE(isa, legacy, isa_driver, 0, 0);

/*
 * Attach the ISA bus to the xenpv bus in order to get syscons.
 */
DRIVER_MODULE(isa, xenpv, isa_driver, 0, 0);
