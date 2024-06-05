#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>

static void alarm_handler(int sig)
{
	if (sig != SIGALRM)
		_exit (2);
}

int main(int argc, char **argv)
{
	--argc;
	++argv;
	assert(argc == 1);

	unsigned int remaining;
	signal (SIGALRM, alarm_handler);
	alarm (1);

	remaining = sleep(1000);
	assert(remaining > 0);

	printf("prefix ");
	puts(argv[0]);

	return 0;
}
