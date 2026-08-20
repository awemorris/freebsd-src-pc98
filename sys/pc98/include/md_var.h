/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef _PC98_INCLUDE_MD_VAR_H_
#define _PC98_INCLUDE_MD_VAR_H_

#include <i386/md_var.h>

/* Some PC-98 CPU boards require cache flushing around ISA DMA. */
extern int need_pre_dma_flush;
extern int need_post_dma_flush;

#endif /* !_PC98_INCLUDE_MD_VAR_H_ */
