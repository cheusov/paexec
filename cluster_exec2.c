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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

#if defined( __NetBSD__) || defined(sun)
#include <getopt.h>
#endif

#define DMALLOC_FUNC_CHECK
#include <maa.h>

#include "wrappers.h"
#include "nonblock_helpers.h"

#define BUFSIZE 2048

void usage ()
{
	printf ("\n\
Usage: cluster_exec [OPTIONS] cmd [files...]\n\
OPTIONS:\n\
      -h --help             give this help\n\
      -V --version          show version\n\
      -v --verbose          verbose mode\n\
      -a --args             list of args (space-separated)\n\
-a is mandatory option\n\
");
}

char *args = NULL;
char *cmd  = NULL;

int verbose = 0;
int count   = 10;

int *fd_in       = NULL;
int *fd_out      = NULL;

//char **buf_in    = NULL;
char **buf_out   = NULL;

//size_t *offs_in  = NULL;
//size_t *offs_out = NULL;

//size_t *size_in  = NULL;
size_t *size_out = NULL;

int *busy        = NULL;
int busy_count   = 0;

pid_t *pids      = NULL;

int max_fd    = 0;

char *buf_stdin   = NULL;
size_t size_stdin = 0;

void init (void)
{
	int i;

	// arrays
	pids  = xmalloc (count * sizeof (*pids));

	fd_in  = xmalloc (count * sizeof (*fd_in));
	fd_out = xmalloc (count * sizeof (*fd_out));

//	buf_in  = xmalloc (count * sizeof (*buf_in));
	buf_out = xmalloc (count * sizeof (*buf_out));

//	offs_in  = xmalloc (count * sizeof (*offs_in));
//	offs_out = xmalloc (count * sizeof (*offs_out));

//	size_in  = xmalloc (count * sizeof (*size_in));
	size_out = xmalloc (count * sizeof (*size_out));

	busy     = xmalloc (count * sizeof (*busy));

	// stdin
	buf_stdin = xmalloc (BUFSIZE);

	// in/out
	for (i=0; i < count; ++i){
//		buf_in  [i] = xmalloc (BUFSIZE);
		buf_out [i] = xmalloc (BUFSIZE);

//		offs_out [i] = 0;
		size_out [i] = 0;

		busy [i] = 0;

		pids [i] = pr_open (
			/* "/home/cheusov/tmp/toupper" */ cmd,
			PR_CREATE_STDIN | PR_CREATE_STDOUT,
			&fd_in [i], &fd_out [i], NULL);

//		nonblock (fd_in [i]);
		nonblock (fd_out [i]);

		if (fd_in [i] > max_fd){
			max_fd = fd_in [i];
		}
		if (fd_out [i] > max_fd){
			max_fd = fd_out [i];
		}
	}

	nonblock (0);
}

int fd_in2index (int fd)
{
	int i;
	for (i=0; i < count; ++i){
		if (fd_in [i] == fd)
			return i;
	}

	err_fatal_errno (__FUNCTION__, "fd_in %d is not found\n", fd);
	return -1;
}

int fd_out2index (int fd)
{
	int i;
	for (i=0; i < count; ++i){
		if (fd_out [i] == fd)
			return i;
	}

	err_fatal_errno (__FUNCTION__, "fd_out %d is not found\n", fd);
	return -1;
}

void write_to_exec (void)
{
	int i;
	for (i=0; i < count; ++i){
		if (!busy [i]){
			if (verbose){
				printf ("send to %d (pid: %d)\n", i, (int) pids [i]);
			}

			busy [i] = 1;
			xwrite (fd_in [i], buf_stdin, strlen (buf_stdin));
			xwrite (fd_in [i], "\n", 1);
			++busy_count;
			return;
		}
	}

	abort ();
}

void loop (void)
{
	fd_set rset;

	int ret = 0;
	int eof = 0;
	int cnt = 0;

	int i, j;

	FD_ZERO (&rset);

	FD_SET (0, &rset);

	while (ret = xselect (max_fd+1, &rset, NULL, NULL, NULL), ret > 0){
		if (verbose){
			printf ("select ret=%d\n", ret);
		}

		// stdin
		if (FD_ISSET (0, &rset)){
			cnt = xread (0, buf_stdin + size_stdin, 1 /*BUFSIZE - size_stdin*/);
			if (cnt){
				for (i=0; i < cnt; ++i){
					if (buf_stdin [size_stdin + i] == '\n'){
						buf_stdin [size_stdin + i] = 0;

						if (verbose){
							printf ("stdin: %s\n", buf_stdin);
						}

						write_to_exec ();

						memmove (buf_stdin,
								 buf_stdin + size_stdin + i + 1,
								 cnt - i - 1);

						size_stdin = 0;
						cnt        -= i + 1;

						i = -1;
						continue;
					}
				}

				size_stdin += cnt;
			}else{
				eof = 1;
			}
		}

		// fd_out
		for (i=0; i < count; ++i){
			if (FD_ISSET (fd_out [i], &rset)){
				cnt = xread (fd_out [i],
							 buf_out [i] + size_out [i],
							 BUFSIZE - size_out [i]);

				if (!cnt){
					err_fatal (__FUNCTION__, "Unexpected eof\n");
				}

				for (j=0; j < cnt; ++j){
					if (buf_out [i] [size_stdin + j] == '\n'){
						buf_out [i] [size_stdin + j] = 0;

						if (!buf_out [i] [0]){
							busy [i] = 0;
							--busy_count;

							if (eof){
								close (fd_in [i]);
							}

							break;
						}

						if (verbose){
							printf ("from pid %d: %s\n", (int) pids [i], buf_out [i]);
						}

						memmove (buf_out [i],
								 buf_out [i] + size_out [i] + j + 1,
								 cnt - j - 1);

						size_out [i] = 0;
						cnt         -= j + 1;

						j = -1;
					}
				}
			}
		}

		// stdin
		FD_CLR (0, &rset);
		if (!eof && busy_count != count){
			FD_SET (0, &rset);
		}

		// fd_out
		for (i=0; i < count; ++i){
			FD_CLR (fd_out [i], &rset);
			if (busy [i]){
				FD_SET (fd_out [i], &rset);
			}
		}

		if (verbose){
			printf ("busy_count = %d\n", busy_count);
			printf ("eof = %d\n", eof);
			for (i=0; i < count; ++i){
				printf ("busy [%d]=%d\n", i, busy [i]);
			}
		}

		if (!busy_count && eof)
			break;
	}

	if (verbose){
		printf ("wait for childs\n");
	}

//	for (i=0; i < count; ++i){
//		pr_wait (pids [i]);
//	}
}

void process_args (int *argc, char ***argv)
{
	int c;

	struct option longopts [] = {
		{ "help",     0, 0, 'h' },
		{ "version",  0, 0, 'V' },
		{ "verbose",  0, 0, 'v' },
		{ "args",     1, 0, 'a' },
		{ NULL,       0, 0, 0 },
	};

	while (c = getopt_long (*argc, *argv, "vhVl", longopts, NULL), c != EOF){
		switch (c) {
			case 'V':
				printf ("cluster_exec v. 0.1\n");
				exit (0);
				break;
			case 'h':
				usage ();
				exit (0);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'a':
				args = strdup (optarg);
				break;
			default:
				usage ();
				exit (1);
		}
	}

	if (optind + 1 != *argc){
		log_error ("", "missing cmd argument");
		exit (1);
	}

	cmd = (*argv) [optind];
}

void log_to_file (void)
{
	char logfile [2000] = "logfile";

	log_file ("cluster_exec2", logfile);
}

int main (int argc, char **argv)
{
	maa_init ("cluster_exec");

//	log_to_file ();

	process_args (&argc, &argv);

	init ();

	log_info ("I started");

	loop ();

	maa_shutdown ();
	return 0;
}
