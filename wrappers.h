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

#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

#include <sys/select.h>
#include <unistd.h>

void nonblock (int fd);
/*
int xselect (
	int nfds,
	fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout);
ssize_t iread (int fd, void *buf, size_t nbytes);
ssize_t xread (int fd, void *buf, size_t nbytes);
ssize_t iwrite (int fd, const void *buf, size_t count);
ssize_t xwrite (int fd, const void *buf, size_t count);
int iclose (int fd);
void xclose (int fd);
*/
char * xfgetln(FILE *fp, size_t *len);
void xsigprocmask (int how, const sigset_t *set, sigset_t *oset);
void xsigaddset (sigset_t *set, int signo);

#endif /* _WRAPPERS_H_ */
