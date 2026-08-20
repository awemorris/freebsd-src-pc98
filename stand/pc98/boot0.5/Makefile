# $FreeBSD: head/sys/boot/pc98/boot0.5/Makefile 309412 2016-12-02 14:44:38Z imp $

PROG=	${BOOT}.out
INTERNALPROG=
FILES=	${BOOT}
MAN=
SRCS=	start.s boot.s boot0.5.s disk.s selector.s support.s syscons.s \
	putssjis.s
CLEANFILES= ${BOOT} ${BOOT}.bin

BOOT=	boot0.5

# The base address that we the boot0 code to to run it.  Don't change this
# unless you are glutton for punishment.
BOOT_BOOT0_ORG?=	0x0000

LDFLAGS=-e start -Ttext ${BOOT_BOOT0_ORG} -Wl,-N,-T,${.CURDIR}/ldscript

# The size of boot0.5 must be 7168 bytes
${BOOT}: ${BOOT}.bin
	cat ${BOOT}.bin /dev/zero | ${DD} of=${BOOT} bs=1 count=7168

${BOOT}.bin: ${BOOT}.out
	${OBJCOPY} -S -O binary ${BOOT}.out ${.TARGET}

.include <bsd.prog.mk>

BOOT05_RAW_MAXSIZE?=	7168
BOOT05_MAXSIZE?=	7168
realall: boot0.5-size-check
boot0.5-size-check: ${BOOT}.bin ${BOOT}
	@raw=`stat -f %z ${BOOT}.bin`; bytes=`stat -f %z ${BOOT}`; \
	    echo "${BOOT}.bin: $$raw/${BOOT05_RAW_MAXSIZE} bytes"; \
	    echo "${BOOT}: $$bytes/${BOOT05_MAXSIZE} bytes"; \
	    test $$raw -le ${BOOT05_RAW_MAXSIZE}; \
	    test $$bytes -le ${BOOT05_MAXSIZE}
