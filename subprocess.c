#include <stdlib.h>
#include <string.h>

#include <maa.h>

#include "wrappers.h"

void put_line (const char *linebuf, void *data)
{
	printf ("line: %s\n", linebuf);
}

typedef void (*line_putter_t) (const char *, void *);

/* returns 1 if eof happens */
int put_until_emptyline (int fd, line_putter_t putter, void *data)
{
	char buf [20];
	ssize_t cnt;

	char *linebuf       = NULL;
	size_t linebuf_size = 0;
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

			if (line_size + cnt >= linebuf_size){
				linebuf = xrealloc (linebuf, linebuf_size + cnt+1);
				linebuf_size += cnt+1;
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
}

int main ()
{
	int proc_fdin, proc_fdout;
	int pid;
	char buf [3000];
	size_t len;

	maa_init ("subprocess");

	pr_open ("/home/cheusov/tmp/list_files",
			 PR_CREATE_STDIN | PR_CREATE_STDOUT,
			 &proc_fdin, &proc_fdout, NULL);

	fprintf (stderr, "pid=%d\n", pid);

	nonblock (proc_fdout);

	while (fgets (buf, sizeof (buf), stdin)){
		len = strlen (buf);
		if (len > 0 && buf [len-1] == '\n'){
			buf [len-1] = 0;
			--len;
		}

		write (proc_fdin, buf, len);
		write (proc_fdin, "\n", 1);

		put_until_emptyline (proc_fdout, put_line, NULL);
	}

	maa_shutdown ();

	return 0;
}
