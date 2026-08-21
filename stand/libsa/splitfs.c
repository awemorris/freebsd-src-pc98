/* 
 * Copyright (c) 2002 Maxim Sobolev
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "stand.h"

#define NTRIES		(3)
#define CONF_BUF	(512)
#define SEEK_BUF	(512)
#define CACHE_BLOCK	(64 * 1024)

struct split_cache
{
    struct split_cache *next;
    size_t used;
    char data[CACHE_BLOCK];
};

struct split_file
{
    char  **filesv;	/* Filenames */
    char  **descsv;	/* Descriptions */
    int	  filesc;	/* Number of parts */
    int	  curfile;	/* Current file number */
    int	  curfd;	/* Current file descriptor */
    off_t tot_pos;	/* Offset from the beginning of the sequence */
    off_t file_pos;	/* Offset from the beginning of the slice */
    struct split_cache *cache;	/* Data read from removable media */
    struct split_cache *cache_tail;
    size_t cache_size;	/* Valid bytes in cache */
};

static int	split_openfile(struct split_file *sf);
static int	splitfs_open(const char *path, struct open_file *f);
static int	splitfs_close(struct open_file *f);
static int	splitfs_read(struct open_file *f, void *buf, size_t size, size_t *resid);
static off_t	splitfs_seek(struct open_file *f, off_t offset, int where);
static int	splitfs_stat(struct open_file *f, struct stat *sb);

static int
split_cache_append(struct split_file *sf, const void *buf, size_t size)
{
    struct split_cache *block;
    size_t amount;

    while (size > 0) {
	block = sf->cache_tail;
	if (block == NULL || block->used == CACHE_BLOCK) {
	    block = malloc(sizeof(*block));
	    if (block == NULL)
		return (ENOMEM);
	    block->next = NULL;
	    block->used = 0;
	    if (sf->cache_tail == NULL)
		sf->cache = block;
	    else
		sf->cache_tail->next = block;
	    sf->cache_tail = block;
	}
	amount = min(size, CACHE_BLOCK - block->used);
	bcopy(buf, block->data + block->used, amount);
	block->used += amount;
	sf->cache_size += amount;
	buf = (const char *)buf + amount;
	size -= amount;
    }
    return (0);
}

static size_t
split_cache_read(struct split_file *sf, off_t offset, void *buf, size_t size)
{
    struct split_cache *block;
    size_t amount, block_offset, copied;

    block = sf->cache;
    while (offset >= CACHE_BLOCK) {
	block = block->next;
	offset -= CACHE_BLOCK;
    }
    copied = 0;
    block_offset = offset;
    while (size > 0 && block != NULL) {
	amount = min(size, block->used - block_offset);
	bcopy(block->data + block_offset, buf, amount);
	buf = (char *)buf + amount;
	size -= amount;
	copied += amount;
	block = block->next;
	block_offset = 0;
    }
    return (copied);
}

struct fs_ops splitfs_fsops = {
	.fs_name = "split",
	.fs_flags = 0,
	.fo_open = splitfs_open,
	.fo_close = splitfs_close,
	.fo_read = splitfs_read,
	.fo_write = null_write,
	.fo_seek = splitfs_seek,
	.fo_stat = splitfs_stat,
	.fo_readdir = null_readdir,
};

static void
split_file_destroy(struct split_file *sf)
{
    int i;
    struct split_cache *block;

    if (sf->filesc > 0) {
	for (i = 0; i < sf->filesc; i++) {
	    free(sf->filesv[i]);
	    free(sf->descsv[i]);
	}
	free(sf->filesv);
	free(sf->descsv);
    }
    while ((block = sf->cache) != NULL) {
	sf->cache = block->next;
	free(block);
    }
    free(sf);
}

static int
split_openfile(struct split_file *sf)
{
    int i;

    for (i = 0;; i++) {
	sf->curfd = open(sf->filesv[sf->curfile], O_RDONLY);
	if (sf->curfd >= 0)
	    break;
	if ((sf->curfd == -1) && (errno != ENOENT))
	    return (errno);
	if (i == NTRIES)
	    return (EIO);
	printf("\nInsert disk labelled %s and press any key...",
	    sf->descsv[sf->curfile]);
	getchar();
	putchar('\n');
    }
    sf->file_pos = 0;
    return (0);
}

static int
splitfs_open(const char *fname, struct open_file *f)
{
    char *buf, *confname, *cp;
    int	conffd;
    struct split_file *sf;
    struct stat sb;

    /* Have to be in "just read it" mode */
    if (f->f_flags != F_READ)
	return(EPERM);

    /* If the name already ends in `.split', ignore it */
    if ((cp = strrchr(fname, '.')) && (!strcmp(cp, ".split")))
	return(ENOENT);

    /* Construct new name */
    confname = malloc(strlen(fname) + 7);
    sprintf(confname, "%s.split", fname);

    /* Try to open the configuration file */
    conffd = open(confname, O_RDONLY);
    free(confname);
    if (conffd == -1)
	return(ENOENT);

    if (fstat(conffd, &sb) < 0) {
	printf("splitfs_open: stat failed\n");
	close(conffd);
	return(ENOENT);
    }
    if (!S_ISREG(sb.st_mode)) {
	printf("splitfs_open: not a file\n");
	close(conffd);
	return(EISDIR);			/* best guess */
    }

    /* Allocate a split_file structure, populate it from the config file */
    sf = malloc(sizeof(struct split_file));
    bzero(sf, sizeof(struct split_file));
    buf = malloc(CONF_BUF);
    while (fgetstr(buf, CONF_BUF, conffd) > 0) {
	cp = buf;
	while ((*cp != '\0') && (isspace(*cp) == 0))
	    cp++;
	if (*cp != '\0') {
	    *cp = '\0';
	    cp++;
	}
	while ((*cp != '\0') && (isspace(*cp) != 0))
	    cp++;
	if (*cp == '\0')
	    cp = buf;
	sf->filesc++;
	sf->filesv = realloc(sf->filesv, sizeof(*(sf->filesv)) * sf->filesc);
	sf->descsv = realloc(sf->descsv, sizeof(*(sf->descsv)) * sf->filesc);
	sf->filesv[sf->filesc - 1] = strdup(buf);
	sf->descsv[sf->filesc - 1] = strdup(cp);
    }
    free(buf);
    close(conffd);

    if (sf->filesc == 0) {
	split_file_destroy(sf);
	return(ENOENT);
    }
    errno = split_openfile(sf);
    if (errno != 0) {
	split_file_destroy(sf);
	return(ENOENT);
    }

    /* Looks OK, we'll take it */
    f->f_fsdata = sf;
    return (0);
}

static int
splitfs_close(struct open_file *f)
{
    int fd;
    struct split_file *sf;

    sf = (struct split_file *)f->f_fsdata;
    fd = sf->curfd;
    split_file_destroy(sf);
    return(close(fd));
}
 
static int 
splitfs_read(struct open_file *f, void *buf, size_t size, size_t *resid)
{
    ssize_t nread;
    size_t cached, totread;
    struct split_file *sf;

    sf = (struct split_file *)f->f_fsdata;
    totread = 0;
    do {
	/* A gzip rewind can revisit data from a floppy already removed. */
	if (sf->tot_pos < (off_t)sf->cache_size) {
	    cached = min(size - totread,
		sf->cache_size - (size_t)sf->tot_pos);
	    cached = split_cache_read(sf, sf->tot_pos, buf, cached);
	    sf->tot_pos += cached;
	    totread += cached;
	    buf = (char *)buf + cached;
	    continue;
	}

	nread = read(sf->curfd, buf, size - totread);

	/* Error? */
	if (nread == -1)
	    return (errno);

	if (nread > 0) {
	    errno = split_cache_append(sf, buf, nread);
	    if (errno != 0)
		return (errno);
	}

	sf->tot_pos += nread;
	sf->file_pos += nread;
	totread += nread;
	buf = (char *)buf + nread;

	if (totread < size) {				/* EOF */
	    if (sf->curfile == (sf->filesc - 1))	/* Last slice */
		break;

	    /* Close previous slice */
	    if (close(sf->curfd) != 0)
		return (errno);

	    sf->curfile++;
	    errno = split_openfile(sf);
	    if (errno)
		    return (errno);
	}
    } while (totread < size);

    if (resid != NULL)
	*resid = size - totread;

    return (0);
}

static off_t
splitfs_seek(struct open_file *f, off_t offset, int where)
{
    int nread;
    size_t resid;
    off_t seek_by, target;
    struct split_file *sf;

    sf = (struct split_file *)f->f_fsdata;

    switch (where) {
    case SEEK_SET:
	target = offset;
	break;
    case SEEK_CUR:
	target = sf->tot_pos + offset;
	break;
    case SEEK_END:
	panic("splitfs_seek: SEEK_END not supported");
	break;
    default:
	errno = EINVAL;
	return (-1);
    }

	if (target < 0) {
	    errno = EINVAL;
	    return (-1);
	}
	if (target <= (off_t)sf->cache_size) {
	    sf->tot_pos = target;
	    return (target);
	}

	seek_by = target - sf->tot_pos;
    if (seek_by > 0) {
	/*
	 * Seek forward - implemented using splitfs_read(), because otherwise we'll be
	 * unable to detect that we have crossed slice boundary and hence
	 * unable to do a long seek crossing that boundary.
	 */
	void *tmp;

	tmp = malloc(SEEK_BUF);
	if (tmp == NULL) {
	    errno = ENOMEM;
	    return (-1);
	}

	nread = 0;
	for (; seek_by > 0; seek_by -= nread) {
	    resid = 0;
	    errno = splitfs_read(f, tmp, min(seek_by, SEEK_BUF), &resid);
	    nread = min(seek_by, SEEK_BUF) - resid;
	    if ((errno != 0) || (nread == 0))
		/* Error or EOF */
		break;
	}
	free(tmp);
	if (errno != 0)
	    return (-1);
    }

    return (sf->tot_pos);
}

static int
splitfs_stat(struct open_file *f, struct stat *sb)
{
    int	result;
    struct split_file *sf = (struct split_file *)f->f_fsdata;

    /* stat as normal, but indicate that size is unknown */
    if ((result = fstat(sf->curfd, sb)) == 0)
	sb->st_size = -1;
    return (result);
}
