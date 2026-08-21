#!/bin/sh

set -eu

if [ "$#" -ne 4 ]; then
	echo "usage: $0 release-tree boot.flp kern1.flp kern2.flp" >&2
	exit 1
fi

base=$(realpath "$1")
boot=$2
kern1=$3
kern2=$4
work=$(mktemp -d /tmp/pc98-kernel-fdd.XXXXXX)
unit=

cleanup()
{
	if [ -n "$unit" ]; then
		mdconfig -d -u "$unit" 2>/dev/null || true
	fi
	case "$work" in
	/tmp/pc98-kernel-fdd.*) rm -rf "$work" ;;
	*) echo "refusing to remove unexpected path: $work" >&2 ;;
	esac
}
trap cleanup EXIT HUP INT TERM

test -x "$base/boot/loader_simp"
test -f "$base/boot/boot"
test -f "$base/boot/kernel/kernel"
test -x "$base/sbin/bsdlabel"
test ! -e "$boot"
test ! -e "$kern1"
test ! -e "$kern2"

mkdir -p "$work/boot/boot/kernel" "$work/kern1/boot/kernel" \
	"$work/kern2/boot/kernel"
install -m 0555 "$base/boot/loader_simp" "$work/boot/boot/loader"
printf '%s\n' \
	'set vfs.root.mountfrom=cd9660:/dev/cd0' \
	'load /boot/kernel/kernel' \
	'boot' > "$work/boot/boot/loader.rc"
printf '%s\n' \
	'/boot/kernel/kernel.gz.boot "Boot floppy"' \
	'/boot/kernel/kernel.gz.aa "Kernel floppy 1"' \
	'/boot/kernel/kernel.gz.ab "Kernel floppy 2"' \
	> "$work/boot/boot/kernel/kernel.gz.split"

# Keep the release kernel intact.  The split points reproduce the traditional
# PC-98 layout: a 16 KiB bootstrap fragment and two kernel floppies.
gzip -n -9c "$base/boot/kernel/kernel" > "$work/kernel.gz"
dd if="$work/kernel.gz" of="$work/boot/boot/kernel/kernel.gz.boot" \
	bs=512 count=32 status=none
dd if="$work/kernel.gz" of="$work/kern1/boot/kernel/kernel.gz.aa" \
	bs=512 skip=32 count=2360 status=none
dd if="$work/kernel.gz" of="$work/kern2/boot/kernel/kernel.gz.ab" \
	bs=512 skip=2392 status=none
test "$(stat -f %z "$work/boot/boot/kernel/kernel.gz.boot")" -eq 16384
test "$(stat -f %z "$work/kern1/boot/kernel/kernel.gz.aa")" -eq 1208320
test -s "$work/kern2/boot/kernel/kernel.gz.ab"

make_image()
{
	tree=$1
	image=$2
	bootable=$3

	makefs -t ffs -B little -o version=1,bsize=4096,fsize=512 \
		-s 1440k "$image" "$tree"
	unit=$(mdconfig -a -t vnode -f "$image")
	printf '%s\n' \
		'8 partitions:' \
		'  a: 2880 0 4.2BSD 512 4096 0' \
		'  c: 2880 0 unused 0 0' > "$work/label"
	if [ "$bootable" = yes ]; then
		"$base/sbin/bsdlabel" -R -B -b "$base/boot/boot" \
			"$unit" "$work/label"
	else
		"$base/sbin/bsdlabel" -R "$unit" "$work/label"
	fi
	fsck_ffs -n "/dev/${unit}a"
	mdconfig -d -u "$unit"
	unit=
	test "$(stat -f %z "$image")" -eq 1474560
}

make_image "$work/boot" "$boot" yes
make_image "$work/kern1" "$kern1" no
make_image "$work/kern2" "$kern2" no

cat "$work/boot/boot/kernel/kernel.gz.boot" \
	"$work/kern1/boot/kernel/kernel.gz.aa" \
	"$work/kern2/boot/kernel/kernel.gz.ab" | gzip -t
sha256 "$boot" "$kern1" "$kern2"

trap - EXIT HUP INT TERM
cleanup
