#!/bin/sh
#
# Build a PC-98 CF/IDE installer image from a populated release tree.
# The release tree must already contain the installer and all desired sets in
# usr/freebsd-dist.  This script does not fetch or alter distribution sets.
#

set -eu

usage()
{
	echo "usage: $0 release-tree output.raw" >&2
	exit 1
}

[ "$#" -eq 2 ] || usage

base=$(realpath "$1")
output=$2
work=$(mktemp -d /tmp/pc98-installer-hdd.XXXXXX)
unit=

cleanup()
{
	if [ -n "$unit" ]; then
		mdconfig -d -u "$unit" 2>/dev/null || true
	fi
	case "$work" in
	/tmp/pc98-installer-hdd.*) rm -rf "$work" ;;
	*) echo "refusing to remove unexpected path: $work" >&2 ;;
	esac
}
trap cleanup EXIT HUP INT TERM

[ -d "$base" ] || usage
[ ! -e "$output" ] || {
	echo "refusing to overwrite $output" >&2
	exit 1
}
test -f "$base/boot/pc98boot"
test -f "$base/boot/boot"
test -f "$base/boot/loader"
test -x "$base/sbin/bsdlabel"
test -f "$base/usr/freebsd-dist/MANIFEST"

# qemu-pc98 and real PC-98 firmware use 8 heads and 17 sectors per track.
# Reserve cylinder zero for the IPL and partition table, and keep the FFS
# partition size aligned to the 64-sector fragment size used below.
cylinders=32768
sectors_per_cylinder=136
total_sectors=$((cylinders * sectors_per_cylinder))
slice_sectors=$((total_sectors - sectors_per_cylinder))
fs_sectors=$((slice_sectors / 64 * 64))
slice_bytes=$((fs_sectors * 512))

makefs -t ffs -B little -o version=2,bsize=32768,fsize=4096 \
	-s "$slice_bytes" "$work/slice.img" "$base"

unit=$(mdconfig -a -t vnode -f "$work/slice.img")
printf '%s\n' \
	'8 partitions:' \
	"  a: $fs_sectors 0 4.2BSD 4096 32768 0" \
	"  c: $fs_sectors 0 unused 0 0" > "$work/label"
"$base/sbin/bsdlabel" -R -B -b "$base/boot/boot" "$unit" "$work/label"
fsck_ffs -n "/dev/${unit}a"
mdconfig -d -u "$unit"
unit=

truncate -s $((total_sectors * 512)) "$output"
dd if="$base/boot/pc98boot" of="$output" bs=512 conv=notrunc status=none
dd if=/dev/zero of="$work/pc98-table" bs=512 count=1 status=none

# One active FreeBSD partition.  PC-98 CHS values are zero-based; the slice
# begins at cylinder 1.  The end cylinder is 32767.  This byte layout matches
# the image used for the standalone installation acceptance test.
printf '\224\304\000\000\000\000\001\000\000\000\001\000\000\000\377\177Installer\000\000\000\000\000\000\000' | \
	dd of="$work/pc98-table" bs=1 conv=notrunc status=none
printf '\125\252' | \
	dd of="$work/pc98-table" bs=1 seek=510 conv=notrunc status=none
dd if="$work/pc98-table" of="$output" bs=512 seek=1 conv=notrunc status=none
dd if="$work/slice.img" of="$output" bs=512 seek="$sectors_per_cylinder" \
	conv=notrunc,sparse status=none

expected_size=$((total_sectors * 512))
# The filesystem size is rounded down to a 64-sector fragment boundary.  Some
# dd implementations discard the zero-filled tail of a pre-sized sparse file,
# so restore the full PC-98 geometry after copying the slice.
truncate -s "$expected_size" "$output"

actual_size=$(stat -f %z "$output")
[ "$actual_size" -eq "$expected_size" ] || {
	echo "image size mismatch: got $actual_size, expected $expected_size" >&2
	exit 1
}

sha256 "$output" > "$output.sha256"
cat "$output.sha256"

trap - EXIT HUP INT TERM
cleanup
