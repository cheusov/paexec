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

#if HAVE_HEADER_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h> /* On ancient HP-UX select(2) is declared here */
#include <unistd.h>
#include <signal.h>

void nonblock (int fd);
char * xfgetln(FILE *fp, size_t *len);
void xsigprocmask (int how, const sigset_t *set, sigset_t *oset);
void xsigaddset (sigset_t *set, int signo);

#endif /* _WRAPPERS_H_ */
