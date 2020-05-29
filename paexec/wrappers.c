/*
 * Copyright (c) 2007-2019 Aleksey Cheusov <vle@gmx.net>
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
#include <signal.h>

#include <maa.h>

#include <mkc_getdelim.h>

#include "decls.h"
#include "wrappers.h"
#include "common.h"

void nonblock(int fd)
{
	int ret = fcntl(fd, F_GETFL, 0);
	if (ret == -1){
		perror("fcntl failed(2)");
		exit(1);
	}

	ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
	if (ret == -1){
		perror("fcntl failed(2)");
		exit(1);
	}
}

void xsigaddset(sigset_t *set, int signo)
{
	if (sigaddset(set, signo)){
		perror("sigaddset(2) failed");
		exit(1);
	}
}

void xsigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	if (sigprocmask(how, set, oset)){
		perror("sigprocmask(2) failed");
		exit(1);
	}
}

ssize_t xgetdelim(char **lineptr, size_t *n, int delimiter, FILE *stream)
{
	ssize_t ret = getdelim(lineptr, n, delimiter, stream);

	if (ret == (ssize_t) -1 && ferror(stdin)){
		perror("getdelim(3) failed");
		exit(1);
	}

	return ret;
}

char *xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (!ret){
		perror("strdup(3) failed");
		exit(1);
	}

	return ret;
}

void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (!ret){
		perror("malloc(3) failed");
		exit(1);
	}

	return ret;
}

void *xcalloc(size_t number, size_t size)
{
	void *ret = calloc(number, size);
	if (!ret){
		perror("calloc(3) failed");
		exit(1);
	}

	return ret;
}

void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);
	if (!ret){
		perror("realloc(3) failed");
		exit(1);
	}

	return ret;
}

void xfree(void *p)
{
	if (p)
		free(p);
}

void xshquote(const char *arg, char *buf, size_t bufsize)
{
	size_t ret = shquote(arg, buf, bufsize);
	if ((size_t)-1 == ret){
		err__internal(__func__, "paexec: shquote(3) failed");
		exit(1);
	}
}

void err__fatal(const char *routine, const char *m)
{
	kill_childs();
	wait_for_childs();

	fflush(stdout);

	err_fatal(routine, m);
}

void err__fatal_errno(const char *routine, const char *m)
{
	kill_childs();
	wait_for_childs();

	fflush(stdout);

	err_fatal_errno(routine, m);
}

void err__internal(const char *routine, const char *m)
{
	kill_childs();
	wait_for_childs();

	fflush(stdout);

	err_internal(routine, m);
}
