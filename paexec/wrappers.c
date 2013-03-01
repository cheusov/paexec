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

char * xfgetln(FILE *fp, size_t *len)
{
	static char *buffer       = NULL;
	static size_t buffer_size = 0;
	int c;
	size_t sz = 0;

	while (c = getc (fp), c != EOF && c != '\n'){
		if (sz+1 >= buffer_size){
			// +2 is enough for `c' and terminating zero
			buffer_size = buffer_size * 3 / 2 + 2;
			buffer = xrealloc (buffer, buffer_size);
		}

		buffer [sz++] = (char) c;
	}

	if (ferror (stdin)){
		log_error ("", "getc failed: %s\n", strerror (errno));
		exit (1);
	}
	if (!sz){
		if (feof (stdin)){
			return NULL;
		}else if (!buffer_size){
			buffer = xmalloc (1);
			buffer_size = 1;
		}
	}

	buffer [sz] = 0;
	*len = sz;
	return buffer;
}
