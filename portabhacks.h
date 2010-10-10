#ifndef _PORTABHACKS_H_
#define _PORTABHACKS_H_

#include <unistd.h>

#if HAVE_FUNC5_GETOPT_LONG_GETOPT_H
#include <getopt.h>
#define HAVE_GETOPT_LONG 1
#elif HAVE_FUNC5_GETOPT_LONG
#define HAVE_GETOPT_LONG 1
#endif

#endif /* _PORTABHACKS_H_ */
