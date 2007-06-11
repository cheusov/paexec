#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <maa.h>

void nonblock (int fd)
{
	int ret = fcntl (fd, F_GETFL, 0);
	if (ret == -1){
		perror ("fcntl failed");
		exit (1);
	}

	ret = fcntl (fd, F_SETFL, ret | O_NONBLOCK);
	if (ret == -1){
		perror ("fcntl failed");
		exit (1);
	}
}

int xselect (
	int nfds,
	fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout)
{
	int ret;
	do {
		ret = select (nfds, readfds, writefds, exceptfds, timeout);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		perror ("select failed");
		exit (1);
	}

	return ret;
}

ssize_t xread (int fd, void *buf, size_t nbytes)
{
	ssize_t ret;
	do {
		ret = read (fd, buf, nbytes);
	}while (ret == -1 && errno == EINTR);

	if (ret == -1){
		perror ("select failed");
		exit (1);
	}

	return ret;
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

void put_line (char *linebuf, size_t linebuf_size, size_t line_size)
{
	linebuf [line_size] = 0;
	printf ("line: %s\n", linebuf);
}

void print (int fdin)
{
	char buf [20];
#if 0
	size_t cnt;
	while (cnt = xread (fdin, buf, sizeof (buf)), cnt != 0){
		write (1, buf, cnt);
	}
#else
	ssize_t cnt;

	char *linebuf       = NULL;
	size_t linebuf_size = 0;
	size_t line_size    = 0;

	char c;
	int i;

	nonblock (fdin);

	fd_set rset;

	for (;;){
		FD_ZERO (&rset);
		FD_SET (fdin, &rset);

//		fprintf (stderr, "select\n");
		xselect (fdin+1, &rset, NULL, NULL, NULL);

//		fprintf (stderr, "check\n");
		if (FD_ISSET (fdin, &rset)){
//			fprintf (stderr, "yes\n");
			cnt = xread (fdin, buf, sizeof (buf));

			if (line_size + cnt >= linebuf_size){
				linebuf = xrealloc (linebuf, linebuf_size + cnt+1);
				linebuf_size += cnt+1;
			}

//			fprintf (stderr, "cnt = %d\n", cnt);
			if (!cnt){
				put_line (linebuf, linebuf_size, line_size);
				return;
			}

			for (i=0; i < cnt; ++i){
				c = buf [i];
				if (c == '\n'){
					put_line (linebuf, linebuf_size, line_size);
					line_size = 0;
				}else{
					linebuf [line_size++] = c;
				}
			}
		}
	}
#endif
}

int main ()
{
	int proc_fdout;
	int pid;

	maa_init ("subprocess");

	pr_open ("/bin/ls -la", PR_CREATE_STDOUT,
			 NULL, &proc_fdout, NULL);

	fprintf (stderr, "pid=%d\n", pid);
	print (proc_fdout);

	maa_shutdown ();

	return 0;
}
