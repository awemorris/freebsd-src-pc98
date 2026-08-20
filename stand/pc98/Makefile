# $FreeBSD$

NO_OBJ=t

.include <bsd.init.mk>

# boot2 consumes the PC-98 BTX artifacts.  Keep the ordering explicit even
# when the surrounding stand tree is built in parallel.
SUBDIR.yes+=	btx
SUBDIR.yes+=	.WAIT
SUBDIR.yes+=	boot0 boot0.5 boot2

.include <bsd.subdir.mk>
