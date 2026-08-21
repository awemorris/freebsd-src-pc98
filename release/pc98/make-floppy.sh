#!/bin/sh
#
# Build a 1.44MB PC-98 staged installer floppy.  The floppy contains only
# the PC-98 loader; the kernel, installation environment, and distribution
# sets are read from the PC-98 slice embedded in the accompanying release ISO.
# Present that ISO as both a SCSI disk (for the loader BIOS) and an ATAPI CD
# (for the kernel).
#

set -eu

if [ "$#" -ne 2 ]; then
	echo "usage: $0 release-tree output.flp" >&2
	exit 1
fi

base=$(realpath "$1")
output=$2
work=$(mktemp -d /tmp/pc98-installer-fdd.XXXXXX)
unit=
cleanup()
{
	if [ -n "$unit" ]; then
		mdconfig -d -u "$unit" 2>/dev/null || true
	fi
	case "$work" in
	/tmp/pc98-installer-fdd.*) rm -rf "$work" ;;
	*) echo "refusing to remove unexpected path: $work" >&2 ;;
	esac
}
trap cleanup EXIT HUP INT TERM

test -x "$base/boot/loader_simp"
test -f "$base/boot/boot"
test -x "$base/sbin/bsdlabel"
test ! -e "$output"

mkdir -p "$work/tree/boot"
cp "$base/boot/loader_simp" "$work/tree/boot/loader"
printf '%s\n' \
	'set currdev=disk1s1:' \
	'set vfs.root.mountfrom=cd9660:/dev/cd0' \
	'load /boot/kernel/kernel' \
	'boot' > "$work/tree/boot/loader.rc"
printf '%s\n' \
	'autoboot_delay="0"' \
	'beastie_disable="YES"' \
	'console="vidconsole"' > "$work/tree/boot/loader.conf"

makefs -t ffs -B little -o version=1,bsize=4096,fsize=512 \
	-s 1440k "$output" "$work/tree"
unit=$(mdconfig -a -t vnode -f "$output")
printf '%s\n' \
	'8 partitions:' \
	'  a: 2880 0 4.2BSD 512 4096 0' \
	'  c: 2880 0 unused 0 0' > "$work/label"
"$base/sbin/bsdlabel" -R -B -b "$base/boot/boot" "$unit" "$work/label"
fsck_ffs -n "/dev/${unit}a"
mdconfig -d -u "$unit"
unit=

test "$(stat -f %z "$output")" -eq 1474560
trap - EXIT HUP INT TERM
cleanup
