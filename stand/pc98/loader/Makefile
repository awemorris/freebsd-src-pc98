# $FreeBSD$

HAVE_ZFS=		no
LOADER_NET_SUPPORT?=	no
LOADER_NFS_SUPPORT?=	no
LOADER_TFTP_SUPPORT?=	no
LOADER_CD9660_SUPPORT?=	no
LOADER_EXT2FS_SUPPORT?=	no
LOADER_MSDOS_SUPPORT?=	no
LOADER_UFS_SUPPORT?=	yes
LOADER_GZIP_SUPPORT?=	no
LOADER_BZIP2_SUPPORT?=	no
MK_LOADER_VERIEXEC:=	no
MK_LOADER_VERIEXEC_PASS_MANIFEST:=	no

.include <bsd.init.mk>

LOADER?=	loader_${LOADER_INTERP}
PROG=		${LOADER}.sym
INTERNALPROG=
NEWVERSWHAT?=	"bootstrap loader" pc98
VERSION_FILE=	${BOOTSRC}/i386/loader/version

.PATH:		${BOOTSRC}/pc98/loader ${BOOTSRC}/i386/loader
SRCS=		main.c conf.c vers.c

HAVE_BCACHE=	yes
HAVE_PNP=	yes
HAVE_ISABUS=	yes
HELP_FILES=	${BOOTSRC}/pc98/loader/help.pc98

.include	"${BOOTSRC}/loader.mk"

CLEANFILES+=	${LOADER} ${LOADER}.bin
ORG=		0x0
CFLAGS+=	-Wall -I${BOOTSRC}/pc98 -I${BOOTSRC}/i386
LDFLAGS+=	-static ${LDFLAGS_ORG} -Wl,--gc-sections

LIBPC98=	${BOOTOBJ}/pc98/libpc98/libpc98.a

${LOADER}: ${LOADER}.bin ${BTXLDR} ${BTXKERN}
	btxld -v -f aout -e ${LOADER_ADDRESS} -o ${.TARGET} -l ${BTXLDR} \
		-b ${BTXKERN} ${LOADER}.bin

${LOADER}.bin: ${LOADER}.sym
	strip -R .comment -R .note -o ${.TARGET} ${.ALLSRC}

FILES+=	${LOADER}
FILESMODE_${LOADER}=	${BINMODE} -b
LDFLAGS+=	${BTXCRT}
DPADD=	${LDR_INTERP32} ${LIBPC98} ${LIBSA32}
LDADD=	${LDR_INTERP32} ${LIBPC98} ${LIBSA32}

.include <bsd.prog.mk>
