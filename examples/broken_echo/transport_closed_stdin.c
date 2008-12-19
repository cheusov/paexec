#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static const char *id;

int main (int argc, char **argv)
{
	char task [1000];

	--argc, ++argv;

	id = argv [0];

	while (fgets (task, sizeof (task), stdin)){
		task [strlen (task)-1] = 0;

		printf ("I'll output %s\n%s\nsuccess\n\n", task, task);

		fflush (stdout);

		if (!strcmp (task, id)){
			fclose (stdin);
			sleep (2);
			exit (1);
		}
	}

	return 0;
}
