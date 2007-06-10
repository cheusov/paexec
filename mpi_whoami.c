#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <mpi.h>

#define TAG_SIZE_OR_END 2
#define TAG_DATA        4

static int rank;
static int count;

typedef enum {
	st_master,
	st_wait,
	st_busy,
	st_dead,
} status_t;

static status_t *status_arr = NULL;

static int count_busy = 0;
static int count_wait = 0;

static int eof = 0;

void master_init ()
{
	int i;
	status_arr = (status_t *) malloc (count * sizeof (status_t));
	if (!status_arr){
		perror ("malloc failed");
		exit (1);
	}

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
		fprintf (stderr, "executor:\n");

		MPI_Recv (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END,
				  MPI_COMM_WORLD, &status);

		fprintf (stderr, "executor: size = %d\n", size);

		if (size < 0)
			return;

		if (size > buf_size){
			buf_size = size;
			buf = realloc (buf, size);
			if (!buf){
				perror ("realloc failed");
				exit (1);
			}
		}

		MPI_Recv (buf, size, MPI_CHAR, 0, TAG_DATA,
				  MPI_COMM_WORLD, &status);

		fprintf (stderr, "executor: buf = %s\n", buf);
#ifndef NDEBUG
		MPI_Get_count (&status, MPI_CHAR, &cnt);
		assert (cnt == size);
#endif

		for (i=0; i < size; ++i){
			buf [i] = toupper ((unsigned char) buf [i]);
		}

		fprintf (stderr, "executor: mpi_send size = %d\n", size);
		MPI_Send (&size, 1, MPI_INT, 0, TAG_SIZE_OR_END, MPI_COMM_WORLD);
		fprintf (stderr, "executor: mpi_send buf = %s\n", buf);
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

	fprintf (stderr, "send to executor: size = %d\n", size);

	MPI_Send (&size, 1, MPI_INT, i, TAG_SIZE_OR_END, MPI_COMM_WORLD);
	fprintf (stderr, "send to executor: line = %s\n", line);
	MPI_Send (line, size, MPI_CHAR, i, TAG_DATA, MPI_COMM_WORLD);

	status_arr [i] = st_busy;

	--count_wait;
	++count_busy;
}

void master ()
{
	MPI_Status status;
	int i, j;
	char *line;
	int size;
	int source;
	char *buf = NULL;
	int buf_size = 0;
	int cnt = 0;

	master_init ();

	while (count_busy > 0 || !eof){
		/* send lines to executors */
		if (count_wait > 0 && !eof){
			fprintf (stderr, "count_wait=%d\n", count_wait);
			for (i=1; i < count; ++i){
				if (status_arr [i] == st_wait){
					line = getnextline ();
					fprintf (stderr, "line=%s\n", line);
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

		/* recieve results from executors */
		if (count_busy > 0){
			fprintf (stderr, "count_busy=%d\n", count_busy);
			MPI_Recv (&size, 1, MPI_INT, MPI_ANY_SOURCE, TAG_SIZE_OR_END,
					  MPI_COMM_WORLD, &status);

			fprintf (stderr, "recv size: %d\n", size);
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

				continue;
			}

			if (size > buf_size){
				buf_size = size;
				buf = realloc (buf, size);
				if (!buf){
					perror ("realloc failed");
					exit (1);
				}
			}

			MPI_Recv (buf, size, MPI_CHAR, source, TAG_DATA,
					  MPI_COMM_WORLD, &status);
			MPI_Get_count (&status, MPI_CHAR, &cnt);

			printf ("cnt = %d, res = %s\n", cnt, buf);
		}
	}
}

int main (int argc, char **argv)
{
	MPI_Init (&argc, &argv);

	MPI_Comm_size (MPI_COMM_WORLD, &count);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	if (rank == 0){
		master ();
	}else{
		executor ();
	}

	MPI_Finalize ();
	return 0;
}
