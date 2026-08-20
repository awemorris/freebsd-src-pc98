# $FreeBSD: head/sys/boot/pc98/btx/btxldr/Makefile 305353 2016-09-03 15:26:28Z imp $

PROG=	btxldr
INTERNALPROG=
MAN=
SRCS=	btxldr.S

CFLAGS+=-DLOADER_ADDRESS=${LOADER_ADDRESS}
CFLAGS+=-I${.CURDIR}/../../../i386/common

.if defined(BTXLDR_VERBOSE)
CFLAGS+=-DBTXLDR_VERBOSE
.endif

ORG=${LOADER_ADDRESS}
LDFLAGS=${LDFLAGS_BIN}

.include <bsd.prog.mk>

# XXX: clang integrated-as doesn't grok .codeNN directives yet
CFLAGS.btxldr.S=	${CLANG_NO_IAS}

BTXLDR_MAXSIZE?=	65536
realall: btxldr-size-check
btxldr-size-check: ${PROG}
	@bytes=`stat -f %z ${PROG}`; \
	    echo "${PROG}: $$bytes/${BTXLDR_MAXSIZE} bytes"; \
	    test $$bytes -le ${BTXLDR_MAXSIZE}
