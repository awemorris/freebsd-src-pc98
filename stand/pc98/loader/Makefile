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
SRCS=		main.c conf.c vers.c gfx_bios.c
CFLAGS.gfx_bios.c+=	-DBIOS_TEXT_ONLY

HAVE_BCACHE=	yes
HAVE_PNP=	yes
HAVE_ISABUS=	yes
HELP_FILES=	${BOOTSRC}/pc98/loader/help.pc98
HELP_FILENAME=	loader.help.pc98

.include	"${BOOTSRC}/loader.mk"

# ${MACHINE} is pc98, so loader.mk cannot infer the 32-bit ELF readers.
SRCS+=		load_elf32.c load_elf32_obj.c reloc_elf32.c

CLEANFILES+=	${LOADER} ${LOADER}.bin
ORG=		0x0
CFLAGS+=	-Wall -I${BOOTSRC}/pc98 -I${BOOTSRC}/i386
LDFLAGS+=	-static ${LDFLAGS_ORG} -Wl,--gc-sections
.if ${LINKER_TYPE} == "lld" && ${LINKER_VERSION} >= 130000
LDFLAGS+=	-Wl,-z,nostart-stop-gc
.endif

LIBPC98=	${BOOTOBJ}/pc98/libpc98/libpc98.a
LIBSA32:=	${LIBSA}

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
