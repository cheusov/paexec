#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

#include <sys/select.h>
#include <unistd.h>

void nonblock (int fd);
int xselect (
	int nfds,
	fd_set * readfds, fd_set * writefds,
	fd_set * exceptfds, struct timeval * timeout);
ssize_t xread (int fd, void *buf, size_t nbytes);
ssize_t xwrite (int fd, const void *buf, size_t count);
void *xmalloc (size_t s);
void *xrealloc (void *p, size_t s);

#endif // _WRAPPERS_H_
