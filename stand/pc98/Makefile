# $FreeBSD$

NO_OBJ=t

.include <bsd.init.mk>

# boot2 consumes the PC-98 BTX artifacts.  Keep the ordering explicit even
# when the surrounding stand tree is built in parallel.
SUBDIR.yes+=	btx libpc98
SUBDIR.yes+=	.WAIT
SUBDIR.yes+=	boot0 boot0.5
SUBDIR.yes+=	.WAIT
SUBDIR.yes+=	pc98boot boot2 cdboot loader_simp

SUBDIR_DEPEND_pc98boot+=	boot0 boot0.5
SUBDIR_DEPEND_loader_simp+=	btx libpc98

.include <bsd.subdir.mk>
