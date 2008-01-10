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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "wrappers.h"
#include "nonblock_helpers.h"

static size_t linebuf_size = 0;
static char *linebuf       = NULL;

int put_until_emptyline (int fd, line_putter_t putter, void *data)
{
#if 0
	char c;
	int len = 0;
	while (xread (fd, &c, 1) == 1){
		if (len + 1 >= linebuf_size){
			linebuf_size = len + 20;
			linebuf = xrealloc (linebuf, linebuf_size);
		}

		if (c == '\n'){
			if (len == 0){
				return 0;
			}

			linebuf [len] = 0;
			putter (linebuf, data);
			len = 0;
		}else{
			linebuf [len] = c;
			++len;
		}
	}

	assert (len < linebuf_size);
	linebuf [len] = 0;

	putter (linebuf, data);
	return 1;
#else
	char buf [20000];
	ssize_t cnt;

	size_t line_size    = 0;

	char c;
	int i;

	fd_set rset;

	for (;;){
		FD_ZERO (&rset);
		FD_SET (fd, &rset);

		xselect (fd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET (fd, &rset)){
			cnt = xread (fd, buf, sizeof (buf));

			if (cnt < 0){
				perror ("Reading from child process failed\n");
				return 1;
			}

			if (line_size + cnt >= linebuf_size){
				linebuf_size += cnt + 1;
				linebuf = xrealloc (linebuf, linebuf_size);
			}

			if (!cnt){
				linebuf [line_size] = 0;
				putter (linebuf, data);
				return 1;
			}

			for (i=0; i < cnt; ++i){
				c = buf [i];
				if (c != '\n'){
					linebuf [line_size++] = c;
				}else if (!line_size){
					return 0;
				}else{
					linebuf [line_size] = 0;
					putter (linebuf, data);
					line_size = 0;
				}
			}
		}
	}
#endif
}
