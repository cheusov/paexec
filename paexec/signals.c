/*
 * Copyright (c) 2007-2011 Aleksey Cheusov <vle@gmx.net>
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

#include <signal.h>
#include <sys/wait.h>

#include "signals.h"
#include "common.h"
#include "wrappers.h"

void block_signals(void)
{
	sigset_t set;

	sigemptyset(&set);
	xsigaddset(&set, SIGALRM);
	xsigaddset(&set, SIGCHLD);

	xsigprocmask(SIG_BLOCK, &set, NULL);
}

void unblock_signals(void)
{
	sigset_t set;

	sigemptyset(&set);
	xsigaddset(&set, SIGALRM);
	xsigaddset(&set, SIGCHLD);

	xsigprocmask(SIG_UNBLOCK, &set, NULL);
}

void set_sig_handler(int sig, void (*handler) (int))
{
	struct sigaction sa;

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(sig, &sa, NULL);
}

void ignore_sigpipe(void)
{
	set_sig_handler(SIGPIPE, SIG_IGN);
}

void wait_for_sigalrm(void)
{
	sigset_t set;

	sigemptyset(&set);

	sigsuspend(&set);
}

void handler_sigchld(int dummy attr_unused)
{
	int status;
	pid_t pid;

	while (pid = waitpid(-1, &status, WNOHANG), pid > 0){
	}
}

void set_sigalrm_handler(void)
{
	set_sig_handler(SIGALRM, handler_sigalrm);
}

void set_sigchld_handler(void)
{
	set_sig_handler(SIGCHLD, handler_sigchld);
}

int sigalrm_tics = 0;
void handler_sigalrm(int dummy attr_unused)
{
	++sigalrm_tics;
	alarm(1);
}
