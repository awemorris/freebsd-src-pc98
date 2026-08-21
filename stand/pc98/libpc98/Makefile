# $FreeBSD$

.include <bsd.init.mk>

LIB=		pc98

.PATH:	${BOOTSRC}/i386/libi386
SRCS=	bio.c biospci.c bootinfo.c bootinfo32.c devicename.c elf32_freebsd.c \
	i386_copy.c i386_module.c nullconsole.c

# PC-98 BIOS, disk, memory and console providers.
SRCS+=	biosdisk.c biosmem.c biossmap.c comconsole.c pc98_sys.c time.c \
	vidconsole.c

BOOT_COMCONSOLE_PORT?=	0x238
CFLAGS+=	-DCOMPORT=${BOOT_COMCONSOLE_PORT}
BOOT_COMCONSOLE_SPEED?=	9600
CFLAGS+=	-DCOMSPEED=${BOOT_COMCONSOLE_SPEED}
CFLAGS+=	-DTERM_EMU -Dalloca=__builtin_alloca
CFLAGS+=	-I${BOOTSRC}/pc98 -I${BOOTSRC}/i386/libi386 \
	-I${LDRSRC} -I${BOOTSRC}/i386/common -I${BTXLIB}
CFLAGS.bootinfo.c+=	-I${SYSDIR}/teken -I${SRCTOP}/contrib/pnglite
CFLAGS+=	-I${ZFSSRC} -I${SYSDIR}/cddl/boot/zfs \
	-I${SYSDIR}/cddl/contrib/opensolaris/uts/common
CFLAGS+=	${FORMAT_EXTENSIONS}

.include <bsd.lib.mk>
