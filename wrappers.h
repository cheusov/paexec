/*
 * Copyright (c) 2006-2007, Aleksey Cheusov <vle@gmx.net>
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation.  I make no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 */

#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

#include <sys/select.h>
#include <unistd.h>

void nonblock (int fd);
int xselect (
	int nfds,
	fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout);
ssize_t xread (int fd, void *buf, size_t nbytes);
ssize_t xwrite (int fd, const void *buf, size_t count);
void *xmalloc (size_t s);
void *xrealloc (void *p, size_t s);
void xfree(void *ptr);
int xclose (int fd);
char *xstrdup (char *s);

#endif // _WRAPPERS_H_
