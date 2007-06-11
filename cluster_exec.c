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

#include <mpi.h>

#define TAG_SIZE_OR_END 2
#define TAG_DATA        4

static int rank;
static int count;

static int verbose  = 0;

static int line_num = 0;
static int show_line_num = 0;

typedef enum {
	st_master,
	st_wait,
	st_busy,
	st_dead,
} status_t;

static status_t *status_arr   = NULL;
static int      *line_num_arr = NULL;

static int count_busy = 0;
static int count_wait = 0;

static int eof = 0;

void *xmalloc (size_t s)
{
	void *p = malloc (s);
	if (!p){
		perror ("malloc failed");
		exit (1);
	}

	return p;
}

void *xrealloc (void *p, size_t s)
{
	p = realloc (p, s);
	if (!p){
		perror ("realloc failed");
		exit (1);
	}

	return p;
}

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

char *getnextline (void)
{
	static char line [2048];
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

void executor ()
{
	MPI_Status status;
	int size;
	char *buf = NULL;
	int buf_size = 0;
	int cnt;
	int i;

	for (;;){
		if (verbose){
			fprintf (stderr, "executor:\n");
		}

		MPI_Recv (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END,
				  MPI_COMM_WORLD, &status);

		if (verbose){
			fprintf (stderr, "executor: size = %d\n", size);
		}

		if (size < 0)
			return;

		if (size > buf_size){
			buf_size = size;
			buf = xrealloc (buf, size);
		}

		MPI_Recv (buf, size, MPI_CHAR, 0, TAG_DATA,
				  MPI_COMM_WORLD, &status);

		if (verbose){
			fprintf (stderr, "executor: buf = %s\n", buf);
		}
#ifndef NDEBUG
		MPI_Get_count (&status, MPI_CHAR, &cnt);
		assert (cnt == size);
#endif

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

		size = -1;
		MPI_Send (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END, MPI_COMM_WORLD);
	}
}

void send_line_to_executor (int i, char *line)
{
	size_t size = strlen (line);
	++size;

	assert (status_arr [i] == st_wait);

	if (verbose){
		fprintf (stderr, "send to executor: size = %d\n", size);
	}

	MPI_Send (&size, 1, MPI_INT, i, TAG_SIZE_OR_END, MPI_COMM_WORLD);

	if (verbose){
		fprintf (stderr, "send to executor: line = %s\n", line);
	}

	MPI_Send (line, size, MPI_CHAR, i, TAG_DATA, MPI_COMM_WORLD);

	status_arr [i] = st_busy;

	--count_wait;
	++count_busy;
}

void master_send_new_task_to_executor ()
{
	int i, j;
	char *line;
	int size;

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
				send_line_to_executor (i, line);
			}else{
				eof = 1;

				for (j=1; j < count; ++j){
					if (status_arr [j] == st_wait){
						size = -1;
						MPI_Send (&size, 1, MPI_INT, j,
								  TAG_SIZE_OR_END, MPI_COMM_WORLD);

						status_arr [j] = st_dead;
						--count_wait;
					}
				}
			}
			break;
		}
	}

	assert (i < count);
}

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
			size = -1;
			MPI_Send (&size, 1, MPI_INT, source,
					  TAG_SIZE_OR_END, MPI_COMM_WORLD);

			status_arr [source] = st_dead;
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
	printf ("%s\n", buf);
}

void master ()
{
	master_init ();

	while (count_busy > 0 || !eof){
		/* send lines to executors */
		if (count_wait > 0 && !eof){
			master_send_new_task_to_executor ();
		}

		/* recieve results from executors */
		if (count_busy > 0){
			master_recv_data_from_executor ();
		}
	}
}

void usage ()
{
	printf ("\n\
Usage: mpi_run -np <N> cluster_exec [OPTIONS] [files...]\n\
OPTIONS:\n\
      -h --help             give this help\n\
      -v --verbose          verbose mode\n\
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

	while (c = getopt_long (*argc, *argv, "vhV", longopts, NULL), c != EOF){
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

	*argc -= optind;
	*argv += optind;
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
		executor ();
	}

	MPI_Finalize ();
	return 0;
}
