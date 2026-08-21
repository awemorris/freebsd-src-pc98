# $FreeBSD: head/sys/boot/pc98/cdboot/Makefile 305353 2016-09-03 15:26:28Z imp $

.include <bsd.init.mk>

PROG=	cdboot
STRIP=
BINMODE=${NOBINMODE}
MAN=
SRCS=	${PROG}.S

CFLAGS+=-I${BOOTSRC}/i386/common

ORG=	0x0000

LDFLAGS+=${LDFLAGS_BIN}

.include <bsd.prog.mk>

# XXX: clang integrated-as doesn't grok .codeNN directives yet
CFLAGS.cdboot.S=	${CLANG_NO_IAS}
