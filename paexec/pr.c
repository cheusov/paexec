/*
  The following code was derived from libmaa written by Rick Faith,
  relicensing was permitted by author
*/

/*
 * Copyright (c) 1996, 2002 Rickard E. Faith (faith@dict.org)
 * Copyright (c) 2013 Aleksey Cheusov <vle@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "wrappers.h"
#include "pr.h"

/* The idea for the max_fd call is from W. Richard Stevens; Advanced
   Programming in the UNIX Environment (Addison-Wesley Publishing Co.,
   1992); page 43.  The implementation here, however, is different from
   that provided by Stevens for his open_max routine. */

struct _pr_Obj {
   int pid;
};

static struct _pr_Obj *_pr_objects = NULL;

static int max_fd (void)
{
	static int maxFd = 0;

	if (maxFd)
		return maxFd;

#if HAVE_FUNC1_SYSCONF && defined(_SC_OPEN_MAX)
	if ((maxFd = sysconf (_SC_OPEN_MAX)) > 0)
		return maxFd;
#endif

	return maxFd = 1024; /* big enough number */
}

static void _pr_init (void)
{
	if (!_pr_objects)
		_pr_objects = xcalloc (max_fd (), sizeof (struct _pr_Obj));
}

int pr_open (const char *command, int flags, int *infd, int *outfd, int *errfd)
{
	int pid;
	int fdin[2];
	int fdout[2];
	int fderr[2];
	int null;

	_pr_init ();

	if (flags & ~(PR_USE_STDIN | PR_USE_STDOUT | PR_USE_STDERR
				  | PR_CREATE_STDIN | PR_CREATE_STDOUT | PR_CREATE_STDERR
				  | PR_STDERR_TO_STDOUT))
		err_internal (__func__, "Illegal flags passed to pr_open");
	if ((flags & PR_USE_STDIN) && (flags & PR_CREATE_STDIN))
		err_internal (__func__, "Cannot both use and create stdin");
	if ((flags & PR_USE_STDOUT) && (flags & PR_CREATE_STDOUT))
		err_internal (__func__, "Cannot both use and create stdout");
	if ((flags & PR_USE_STDERR) && (flags & PR_CREATE_STDERR))
		err_internal (__func__, "Cannot both use and create stderr");
	if ((flags & PR_STDERR_TO_STDOUT)
		&& ((flags & PR_USE_STDERR) || (flags & PR_CREATE_STDERR)))
		err_internal (__func__, "Cannot use/create stderr when duping to stdout");

	if ((flags & PR_CREATE_STDIN) && pipe (fdin) < 0)
		err_fatal_errno ("paexec: Cannot create pipe for stdin" );
	if ((flags & PR_CREATE_STDOUT) && pipe (fdout) < 0)
		err_fatal_errno ("paexec: Cannot create pipe for stdout" );
	if ((flags & PR_CREATE_STDERR) && pipe (fderr) < 0)
		err_fatal_errno ("paexec: Cannot create pipe for stderr" );

	if ((pid = fork ()) < 0)
		err_fatal_errno ("paexec: Cannot fork" );

	if (pid == 0) {		/* child */
		int i;

#define CHILD(CREATE,USE,fds,writefd,readfd,fd,FILENO,flag)		\
		if (flags & CREATE){									\
			close (fds [writefd]);								\
			dup2 (fds [readfd], FILENO);						\
			close (fds [readfd]);								\
		}else if (flags & USE) {								\
			if (fd && *fd){										\
				dup2 (*fd, FILENO);								\
				close (*fd);									\
			}else{												\
				if ((null = open ("/dev/null", flag)) >= 0) {	\
					dup2 (null, FILENO);						\
					close (null);								\
				}												\
			}													\
		}

		CHILD (PR_CREATE_STDIN, PR_USE_STDIN, fdin, 1, 0, infd,
			   STDIN_FILENO, O_RDONLY);
		CHILD (PR_CREATE_STDOUT, PR_USE_STDOUT, fdout, 0, 1, outfd,
			   STDOUT_FILENO, O_WRONLY);
		CHILD (PR_CREATE_STDERR, PR_USE_STDERR, fderr, 0, 1, errfd,
			   STDERR_FILENO, O_WRONLY);

#undef CHILD
		if (flags & PR_STDERR_TO_STDOUT)
			dup2 (STDOUT_FILENO, STDERR_FILENO);

		for (i = 0; i < max_fd(); i++)
			if (_pr_objects [i].pid > 0)
				close (i);

		execl ("/bin/sh", "/bin/sh", "-c", command, NULL);
		_exit (127);
	}
	/* parent */
#define PARENT(CREATE,USE,fds,readfd,writefd,fd,flag,name)  \
	if (flags & CREATE) {									\
		close (fds [readfd]);								\
		*fd = fds [writefd];								\
		_pr_objects [*fd].pid = pid;						\
	}else if (flags & USE) {								\
		if (fd && *fd) {									\
			_pr_objects [*fd].pid =0;						\
			close (*fd);									\
		}													\
	}

	PARENT (PR_CREATE_STDIN,  PR_USE_STDIN, fdin,  0, 1,
			infd,  "w", "stdin");
	PARENT (PR_CREATE_STDOUT, PR_USE_STDOUT, fdout, 1, 0,
			outfd, "r", "stdout");
	PARENT (PR_CREATE_STDERR, PR_USE_STDERR, fderr, 1, 0,
			errfd, "r", "stderr");

#undef PARENT
	return pid;
}
