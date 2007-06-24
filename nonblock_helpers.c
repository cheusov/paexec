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
		if (len >= linebuf_size){
			++linebuf_size;
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
		}

		++len;
	}
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

//		fprintf (stderr, "select\n");
		xselect (fd+1, &rset, NULL, NULL, NULL);

//		fprintf (stderr, "check\n");
		if (FD_ISSET (fd, &rset)){
//			fprintf (stderr, "yes\n");
			cnt = xread (fd, buf, sizeof (buf));

			if (cnt < 0){
				perror ("Reading from child process failed\n");
				return 1;
			}

			if (line_size + cnt >= linebuf_size){
				linebuf_size += cnt + 1;
				linebuf = xrealloc (linebuf, linebuf_size);
			}

//			fprintf (stderr, "cnt = %d\n", cnt);
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

	abort (); /* this should not happen */
	return 1;
#endif
}