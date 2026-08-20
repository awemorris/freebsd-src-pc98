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

BOOT0_MAXSIZE?=	512
all: boot0-size-check
boot0-size-check: ${BOOT}
	@bytes=`stat -f %z ${BOOT}`; \
	    echo "${BOOT}: $$bytes/${BOOT0_MAXSIZE} bytes"; \
	    test $$bytes -le ${BOOT0_MAXSIZE}
