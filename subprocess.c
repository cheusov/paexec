#include <stdlib.h>
#include <string.h>

#include <maa.h>

#include "wrappers.h"
#include "nonblock_helpers.h"

void put_line (char *linebuf, void *data)
{
	printf ("line: %s\n", linebuf);
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
