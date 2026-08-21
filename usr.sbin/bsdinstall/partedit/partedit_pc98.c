/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2011 Nathan Whitehorn
 * All rights reserved.
 */

#include <string.h>

#include "partedit.h"

const char *
default_scheme(void)
{
	return ("PC98");
}

int
is_scheme_bootable(const char *part_type)
{
	return (strcmp(part_type, "BSD") == 0 ||
	    strcmp(part_type, "PC98") == 0);
}

int
is_fs_bootable(const char *part_type, const char *fs)
{
	return (strcmp(fs, "freebsd-ufs") == 0);
}

size_t
bootpart_size(const char *part_type)
{
	return (0);
}

const char *
bootpart_type(const char *scheme, const char **mountpoint)
{
	return ("freebsd-boot");
}

const char *
bootcode_path(const char *part_type)
{
	if (strcmp(part_type, "PC98") == 0)
		return ("/boot/pc98boot");
	if (strcmp(part_type, "BSD") == 0)
		return ("/boot/boot");
	return (NULL);
}

const char *
partcode_path(const char *part_type, const char *fs_type)
{
	return (NULL);
}
