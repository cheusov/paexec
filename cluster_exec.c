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

#ifdef __NetBSD__
#include <getopt.h>
#endif

#include <mpi.h>

#define DMALLOC_FUNC_CHECK
#include <maa.h>

#include "wrappers.h"
#include "nonblock_helpers.h"

#define TAG_SIZE_OR_END 2
#define TAG_DATA        4

static int rank;
static int count;

static int verbose  = 0;

static int line_num = 0;
static int show_line_num = 0;

static const int minus_one = -1;

static pid_t pid = -1;
static int proc_fdin, proc_fdout;

static char *cmd;

typedef enum {
	st_master,
	st_wait,
	st_busy,
	st_dead,
} status_t;

static status_t *status_arr   = NULL;
static int      *line_num_arr = NULL;

static int count_dead = 0;
static int count_busy = 0;
static int count_wait = 0;

static int eof = 0;

/* */
void master_init ()
{
	int i;
	status_arr   = (status_t *) xmalloc (count * sizeof (*status_arr));
	line_num_arr = (int *)      xmalloc (count * sizeof (*line_num_arr));

	status_arr [0] = st_master;
	for (i=1; i < count; ++i){
		status_arr [i] = st_wait;
	}

	count_busy = 0;
	count_wait = count-1;
}

/* returns next line from stdin */
char *getnextline (void)
{
	static char line [204800];
	size_t len;

	if (fgets (line, sizeof (line), stdin)){
		++line_num;

		len = strlen (line);
		if (len > 0 && line [len-1] == '\n'){
			line [len-1] = '\0';
		}

		return line;
	}else if (ferror (stdin)){
		perror ("fgets faild");

		exit (1);

		return NULL; /* no compilation warnings! */
	}else{
		return NULL; /* eof */
	}
}

void executor_send_result (char *line, void *data)
{
	int size = (int) strlen (line) + 1;

	MPI_Send (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END, MPI_COMM_WORLD);
	MPI_Send (line, size, MPI_CHAR, 0, TAG_DATA,  MPI_COMM_WORLD);
}

/* process data and send results back to the master */
void executor_process_and_send (char *buf, int size)
{
#if 1
	int len = (int) strlen (buf);

	write (proc_fdin, buf, len);
	write (proc_fdin, "\n", 1);

	if (put_until_emptyline (proc_fdout, executor_send_result, NULL)){
		fprintf (stderr, "Child process exited unexpectedly\n");
		exit (0);
	}
#else
	int i;
	for (i=0; i < size; ++i){
		buf [i] = toupper ((unsigned char) buf [i]);
	}

	if (verbose){
		fprintf (stderr, "executor: mpi_send size = %d\n", size);
	}

	MPI_Send (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END, MPI_COMM_WORLD);

	if (verbose){
		fprintf (stderr, "executor: mpi_send buf = %s\n", buf);
	}

	MPI_Send (buf, size, MPI_CHAR, 0, TAG_DATA,  MPI_COMM_WORLD);
#endif
}

void executor_recieve_cmd ()
{
	MPI_Status status;
	int size;

	MPI_Recv (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END,
			  MPI_COMM_WORLD, &status);

	cmd = xmalloc (size);

	MPI_Recv (cmd, size, MPI_CHAR, 0, TAG_DATA,
			  MPI_COMM_WORLD, &status);
}

void executor_subprocess ()
{
	assert (cmd);
	pid = pr_open (cmd,
				   PR_CREATE_STDIN | PR_CREATE_STDOUT,
				   &proc_fdin, &proc_fdout, NULL);
	nonblock (proc_fdout);
}

/* executor process */
void executor ()
{
	MPI_Status status;
	int size;
	char *buf = NULL;
	int buf_size = 0;
	int cnt;

	executor_recieve_cmd ();
	executor_subprocess ();

	for (;;){
		if (verbose){
			fprintf (stderr, "executor:\n");
		}

		/* reading the line size */
		MPI_Recv (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END,
				  MPI_COMM_WORLD, &status);

		if (verbose){
			fprintf (stderr, "executor: size = %d\n", size);
		}

		if (size < 0){
			/* no more lines to be processed */
			return;
		}

		/* */
		if (size > buf_size){
			buf_size = size;
			buf = xrealloc (buf, size);
		}

		/* reading the line */
		MPI_Recv (buf, size, MPI_CHAR, 0, TAG_DATA,
				  MPI_COMM_WORLD, &status);

		if (verbose){
			fprintf (stderr, "executor: buf = %s\n", buf);
		}
#ifndef NDEBUG
		MPI_Get_count (&status, MPI_CHAR, &cnt);
		assert (cnt == size);
#endif

		/* process data and send results back to the master */
		executor_process_and_send (buf, size);

		/* end of line processing,
		   needs further lines to be processed
		*/
		MPI_Send ((void*) &minus_one, 1, MPI_INT, 0,
				  TAG_SIZE_OR_END, MPI_COMM_WORLD);
	}
}

/* send line to executor and mark it as busy */
void master_send_line_to_executor (int i, char *line)
{
	int size = strlen (line);
	++size;

	assert (status_arr [i] == st_wait);

	if (verbose){
		fprintf (stderr, "send to executor: size = %d\n", size);
	}

	line_num_arr [i] = line_num;
	MPI_Send (&size, 1, MPI_INT, i, TAG_SIZE_OR_END, MPI_COMM_WORLD);

	if (verbose){
		fprintf (stderr, "send to executor: line = %s\n", line);
	}

	MPI_Send (line, size, MPI_CHAR, i, TAG_DATA, MPI_COMM_WORLD);

	status_arr [i] = st_busy;

	--count_wait;
	++count_busy;
}

/* mark executor as dead, when there are no new tasks for it */
void master_mark_executor_dead (int num)
{
	MPI_Send ((void *) &minus_one, 1, MPI_INT, num,
			  TAG_SIZE_OR_END, MPI_COMM_WORLD);

	status_arr [num] = st_dead;
	++count_dead;
}

/* next line to free executor */
void master_send_new_task_to_executor ()
{
	int i, j;
	char *line;

	if (verbose){
		fprintf (stderr, "count_wait=%d\n", count_wait);
	}

	for (i=1; i < count; ++i){
		if (status_arr [i] == st_wait){
			line = getnextline ();

			if (verbose){
				fprintf (stderr, "line=%s\n", line);
			}

			if (line){
				master_send_line_to_executor (i, line);
			}else{
				eof = 1;

				for (j=1; j < count; ++j){
					if (status_arr [j] == st_wait){
						master_mark_executor_dead (j);
						--count_wait;
					}
				}

				assert (count_wait == 0);
			}
//			break;
		}
	}

//	assert (i < count);
}

/* read results from any executor */
void master_recv_data_from_executor ()
{
	MPI_Status status;
	char *buf = NULL;
	int buf_size = 0;
	int size;
	int source;
	int cnt = 0;

	if (verbose){
		fprintf (stderr, "count_busy=%d\n", count_busy);
	}

	MPI_Recv (&size, 1, MPI_INT, MPI_ANY_SOURCE, TAG_SIZE_OR_END,
			  MPI_COMM_WORLD, &status);

	if (verbose){
		fprintf (stderr, "recv size: %d\n", size);
	}

	source = status.MPI_SOURCE;

	if (size < 0){
		--count_busy;

		if (eof){
			master_mark_executor_dead (source);
		}else{
			status_arr [source] = st_wait;
			++count_wait;
		}

		return;
	}

	if (size > buf_size){
		buf_size = size;
		buf = xrealloc (buf, size);
	}

	MPI_Recv (buf, size, MPI_CHAR, source, TAG_DATA,
			  MPI_COMM_WORLD, &status);
#ifndef NDEBUG
	MPI_Get_count (&status, MPI_CHAR, &cnt);
	assert (cnt == size);
#endif

	if (verbose){
		printf ("source = %d ", source);
	}
	if (show_line_num){
		printf ("%d ", line_num_arr [source]);
	}
	printf ("%s\n", buf);
}

void master_send_cmd ()
{
	int size;
	int i;

	size = (int) strlen (cmd) + 1;

	for (i=1; i < count; ++i){
		MPI_Send (&size, 1, MPI_INT, i, TAG_SIZE_OR_END,
				  MPI_COMM_WORLD);
		MPI_Send (cmd, size, MPI_CHAR, i, TAG_DATA,
				  MPI_COMM_WORLD);
	}
}

void progress (const char *p)
{
	fprintf (stderr, "%s : %d + %d + %d == %d\n",
			 p,
			count_wait, count_busy, count_dead,
			count_wait + count_busy + count_dead);
}

/* master process */
void master ()
{
	master_init ();
	master_send_cmd ();

	while (count_busy > 0 || !eof){
//		progress ("1");
		assert (count_wait + count_busy + count_dead == count - 1);

		/* send lines to executors */
		if (count_wait > 0 && !eof){
			master_send_new_task_to_executor ();
		}

//		progress ("2");
		assert (count_wait + count_busy + count_dead == count - 1);

		/* recieve results from executors */
		if (count_busy > 0){
			master_recv_data_from_executor ();
		}

//		progress ("3");
		assert (count_wait + count_busy + count_dead == count - 1);
	}
}

void usage ()
{
	printf ("\n\
Usage: mpi_run -np <N> cluster_exec [OPTIONS] cmd [files...]\n\
OPTIONS:\n\
      -h --help             give this help\n\
      -v --verbose          verbose mode\n\
      -l --line-num         show line numbers\n\
");
}

void process_args (int *argc, char ***argv)
{
	int c;

	struct option longopts [] = {
		{ "help",     0, 0, 'h' },
		{ "version",  0, 0, 'V' },
		{ "verbose",  0, 0, 'v' },
		{ "line-num", 0, 0, 'l' },
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
			case 'l':
				show_line_num = 1;
				break;
			default:
				usage ();
				exit (1);
		}
	}

	if (optind + 1 != *argc){
		fprintf (stderr, "missing cmd argument\n");
		exit (1);
	}

	cmd = (*argv) [optind];
}

int main (int argc, char **argv)
{
	MPI_Init (&argc, &argv);

	MPI_Comm_size (MPI_COMM_WORLD, &count);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	if (rank == 0){
		process_args (&argc, &argv);
		master ();
	}else{
		maa_init ("cluster_exec");

		executor ();

		pr_close (proc_fdin);

		maa_shutdown ();
	}

	MPI_Finalize ();
	return 0;
}
