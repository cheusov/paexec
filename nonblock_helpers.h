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

#ifndef _NONBLOCK_HELPERS_H_
#define _NONBLOCK_HELPERS_H_

typedef void (*line_putter_t) (char *, void *);

/* returns 1 if eof happens */
int put_until_emptyline (int fd, line_putter_t putter, void *data);

#endif // _NONBLOCK_HELPERS_H_
