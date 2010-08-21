#ifndef _PORTABHACKS_H_
#define _PORTABHACKS_H_

#ifdef __linux__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /*  _GNU_SOURCE */

#include <getopt.h>

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <getopt.h>
#include <unistd.h>
#else /* neither of platforms supporting getopt_long */
#include <unistd.h>
struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

#define getopt_long(argc, argv, shrt_opts, long_opts, index) \
	getopt(argc, argv, shrt_opts)

#endif /* endof __linux__ */

#endif /* _PORTABHACKS_H_ */
