#ifndef _PORTABHACKS_H_
#define _PORTABHACKS_H_

#if defined(__linux__) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#include <getopt.h>
#include <unistd.h>

#ifndef HAVE_GETOPT_LONG
#define HAVE_GETOPT_LONG 1
#endif

#endif /* endof __linux__ */

#endif /* _PORTABHACKS_H_ */
