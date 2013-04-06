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
#include <signal.h>

#include <maa.h>

#include "wrappers.h"
#include "common.h"

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

void xsigaddset (sigset_t *set, int signo)
{
	if (sigaddset (set, signo)){
		log_error ("", "sigaddset(2) failed: %s\n", strerror (errno));
	}
}

void xsigprocmask (int how, const sigset_t *set, sigset_t *oset)
{
	if (sigprocmask (how, set, oset)){
		log_error ("", "sigaddset(2) failed: %s\n", strerror (errno));
	}
}

ssize_t xgetline(char** lineptr, size_t* n, FILE* stream)
{
	ssize_t ret = getline (lineptr, n, stream);

	if (ret == (ssize_t) -1 && ferror (stdin)){
		log_error ("", "getline(3) failed: %s\n", strerror (errno));
		exit (1);
	}

	return ret;
}
