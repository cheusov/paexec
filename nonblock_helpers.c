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
