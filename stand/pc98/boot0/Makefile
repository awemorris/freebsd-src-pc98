# $FreeBSD: head/sys/boot/pc98/boot0/Makefile 305353 2016-09-03 15:26:28Z imp $

PROG=	${BOOT}
INTERNALPROG=
FILES=	${BOOT}
MAN=
SRCS=	${BOOT}.s
CLEANFILES= ${BOOT}

BOOT=	boot0

# The base address that we the boot0 code to to run it.  Don't change this
# unless you are glutton for punishment.
BOOT_BOOT0_ORG?=	0x0000
ORG=${BOOT_BOOT0_ORG}

LDFLAGS=${LDFLAGS_BIN}

.include <bsd.prog.mk>
