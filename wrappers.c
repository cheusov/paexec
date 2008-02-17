/*
 * Copyright (c) 2007-2008 Aleksey Cheusov <vle@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
	ssize_t ret = 0;
	const char *b = (const char *) buf;

	while (count){
		do {
			ret = write (fd, b, count);
		}while (ret == -1 && errno == EINTR);

		if (ret > 0){
			count -= ret;
			b     += ret;
		}
	}

	if (ret == -1){
		log_error ("", "write failed: %s\n", strerror (errno));
		exit (1);
	}

	return ret;
}

int xclose (int fd)
{
	int ret;
	do {
		ret = close (fd);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		log_error ("", "close failed: %s\n", strerror (errno));
		exit (1);
	}

	return ret;
}
