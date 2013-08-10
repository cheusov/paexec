/*
 * Copyright (c) 1996, 2002 Rickard E. Faith (faith@dict.org)
 * Copyright (c) 2013 Aleksey Cheusov <vle@gmx.net>
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

#define PR_USE_STDIN        0x00000001
#define PR_USE_STDOUT       0x00000002
#define PR_USE_STDERR       0x00000004
#define PR_CREATE_STDIN     0x00000010
#define PR_CREATE_STDOUT    0x00000020
#define PR_CREATE_STDERR    0x00000040
#define PR_STDERR_TO_STDOUT 0x00000100

int pr_open (const char *command, int flags,
		     int *infd, int *outfd, int *errfd);
int pr_close (int fd);
