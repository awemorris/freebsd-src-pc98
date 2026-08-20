# Common definitions for stand/pc98.
# $FreeBSD$

.include "bsd.linker.mk"

LOADER_ADDRESS?=0x200000
CFLAGS+=	-march=i386 -ffreestanding ${CFLAGS_NO_SIMD} -msoft-float
CFLAGS+=	-Os -DPC98
CFLAGS.gcc+=	-mpreferred-stack-boundary=2
LDFLAGS+=	-nostdlib
LDFLAGS.lld+=	-Wl,--no-rosegment
MK_PIE:=	no

BTXDIR=		${BOOTOBJ}/pc98/btx
BTXLDR=		${BTXDIR}/btxldr/btxldr
BTXKERN=	${BTXDIR}/btx/btx
BTXCRT=		${BTXDIR}/lib/crt0.o

BTXSRC=		${BOOTSRC}/pc98/btx
BTXLIB=		${BTXSRC}/lib
CFLAGS+=	-I${BTXLIB}

# PC-98 boot2 still shares the i386 compact binary linker script.  This is
# a linker-format dependency only; BIOS disk and console semantics remain
# PC-98-specific.
LDSCRIPT=	${BOOTSRC}/i386/boot.ldscript
LDFLAGS_ORG=	-Wl,--defsym,ORG=${ORG},-T,${LDSCRIPT}
LDFLAGS_BIN=	-e start ${LDFLAGS_ORG} -Wl,-N,-S,--oformat,binary
.if ${LINKER_FEATURES:Mbuild-id} != ""
LDFLAGS_BIN+=	-Wl,--build-id=none
.endif
LD_FLAGS_BIN=	-static -N --gc-sections

.include "../Makefile.inc"
