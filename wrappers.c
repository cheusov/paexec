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

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define DMALLOC_FUNC_CHECK
#include <maa.h>

#include "wrappers.h"

void nonblock (int fd)
{
	int ret = fcntl (fd, F_GETFL, 0);
	if (ret == -1){
		log_error ("", "fcntl failed: %s\n", strerror (errno));
		exit (1);
	}

	ret = fcntl (fd, F_SETFL, ret | O_NONBLOCK);
	if (ret == -1){
		log_error ("", "fcntl failed: %s\n", strerror (errno));
		exit (1);
	}
}

int xselect (
	int nfds,
	fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout)
{
	int ret;
	do {
		ret = select (nfds, readfds, writefds, exceptfds, timeout);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		log_error ("", "select failed: %s\n", strerror (errno));
		exit (1);
	}

	return ret;
}

ssize_t xread (int fd, void *buf, size_t nbytes)
{
	ssize_t ret;
	do {
		ret = read (fd, buf, nbytes);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		log_error ("", "read failed: %s\n", strerror (errno));
		exit (1);
	}

	return ret;
}

ssize_t xwrite (int fd, const void *buf, size_t count)
{
	ssize_t ret;
	do {
		ret = write (fd, buf, count);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		log_error ("", "write failed: %s\n", strerror (errno));
		exit (1);
	}

	return ret;
}

void *xmalloc (size_t s)
{
	void *p = malloc (s);
	if (!p){
		log_error ("", "malloc failed: %s\n", strerror (errno));
		exit (1);
	}

	return p;
}

void *xrealloc (void *p, size_t s)
{
	p = realloc (p, s);
	if (!p){
		log_error ("", "realloc failed: %s\n", strerror (errno));
		exit (1);
	}

	return p;
}

void xfree(void *ptr)
{
	if (ptr)
		free (ptr);
}
