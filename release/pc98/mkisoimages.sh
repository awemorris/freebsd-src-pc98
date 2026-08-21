#!/bin/sh
#
# Module: mkisoimages.sh
# Author: Jordan K Hubbard
# Date:   22 June 2001
#
#
# This script is used by release/Makefile to build the (optional) ISO images
# for a FreeBSD release.  It is considered architecture dependent since each
# platform has a slightly unique way of making bootable CDs.  This script
# is also allowed to generate any number of images since that is more of
# publishing decision than anything else.
#
# Usage:
#
# mkisoimages.sh [-b] image-label image-name base-bits-dir [extra-bits-dir]
#
# Where -b is passed if the ISO image should be made "bootable" by
# whatever standards this architecture supports (may be unsupported),
# image-label is the ISO image label, image-name is the filename of the
# resulting ISO image, base-bits-dir contains the image contents and
# extra-bits-dir, if provided, contains additional files to be merged
# into base-bits-dir as part of making the image.

set -e

scriptdir=$(dirname $(realpath $0))
. ${scriptdir}/../scripts/tools.subr

if [ "$1" = "-b" ]; then
	MAKEFSARG="$4"
else
	MAKEFSARG="$3"
fi

if [ -f ${MAKEFSARG} ]; then
	BASEBITSDIR=`dirname ${MAKEFSARG}`
	METALOG=${MAKEFSARG}
elif [ -d ${MAKEFSARG} ]; then
	BASEBITSDIR=${MAKEFSARG}
	METALOG=
else
	echo "${MAKEFSARG} must exist"
	exit 1
fi

if [ "$1" = "-b" ]; then
	# PC-98 firmware consumes cdboot as a generic boot image, rather than
	# through the El Torito path used by IBM-PC compatible i386 systems.
	bootable="-o generic-bootimage=$BASEBITSDIR/boot/cdboot"
	pc98_hybrid=yes
	shift
else
	bootable=""
	pc98_hybrid=no
fi

if [ $# -lt 3 ]; then
	echo "Usage: $0 [-b] image-label image-name base-bits-dir [extra-bits-dir]"
	exit 1
fi

LABEL=`echo "$1" | tr '[:lower:]' '[:upper:]'`; shift
NAME="$1"; shift
# MAKEFSARG extracted already
shift

publisher="The FreeBSD Project.  https://www.FreeBSD.org/"
echo "/dev/iso9660/$LABEL / cd9660 ro 0 0" > "$BASEBITSDIR/etc/fstab"
if [ -n "${METALOG}" ]; then
	metalogfilename=$(mktemp /tmp/metalog.XXXXXX)
	cat ${METALOG} > ${metalogfilename}
	echo "./etc/fstab type=file uname=root gname=wheel mode=0644" >> ${metalogfilename}
	MAKEFSARG=${metalogfilename}
fi
${MAKEFS} -D -N ${BASEBITSDIR}/etc -t cd9660 $bootable -o rockridge -o label="$LABEL" -o publisher="$publisher" "$NAME" "$MAKEFSARG" "$@"

if [ "$pc98_hybrid" = yes ]; then
	# The staged floppy reads the ISO through the PC-9801-92 SCSI disk BIOS.
	# Put a non-386BSD PC-98 slice over the complete ISO so libpc98 exposes
	# disk1s1 directly instead of looking for a BSD disklabel.  ISO9660 leaves
	# sectors 0--15 available as its system area, so this does not change the
	# filesystem seen by the kernel's ATAPI CD device.
	sectors_per_cylinder=256	# PC-9801-92 geometry: 8 heads x 32 sectors
	bytes=$(stat -f %z "$NAME")
	sectors=$(((bytes + 511) / 512))
	end_cylinder=$(((sectors + sectors_per_cylinder - 1) /
	    sectors_per_cylinder - 1))
	if [ "$end_cylinder" -gt 65535 ]; then
		echo "$NAME is too large for the PC-98 SCSI BIOS geometry" >&2
		exit 1
	fi
	table=$(mktemp /tmp/pc98-iso-table.XXXXXX)
	dd if=/dev/zero of="$table" bs=512 count=1 status=none
	# Bootable/active FAT16-compatible type: direct slice, cylinder 0..end.
	printf '\241\301\000\000\000\000\000\000\000\000\000\000\000\000' | \
	    dd of="$table" bs=1 conv=notrunc status=none
	printf "\\$(printf '%03o' $((end_cylinder & 255)))\\$(printf '%03o' \
	    $(((end_cylinder >> 8) & 255)))" | \
	    dd of="$table" bs=1 seek=14 conv=notrunc status=none
	printf 'FreeBSD\000\000\000\000\000\000\000\000\000' | \
	    dd of="$table" bs=1 seek=16 conv=notrunc status=none
	# The SCSI ROM probes disks before the main BIOS honors floppy boot order.
	# RETF makes that probe return; boot continues from the staged floppy.
	printf '\313' | dd of="$NAME" bs=1 seek=0 conv=notrunc status=none
	printf '\125\252' | dd of="$NAME" bs=1 seek=510 conv=notrunc status=none
	dd if="$table" of="$NAME" bs=512 seek=1 conv=notrunc status=none
	rm -f "$table"
fi
rm -f "$BASEBITSDIR/etc/fstab"
if [ -n "${METALOG}" ]; then
	rm ${metalogfilename}
fi
