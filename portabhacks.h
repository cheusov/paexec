#ifndef _PORTABHACKS_H_
#define _PORTABHACKS_H_

#ifdef __linux__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#else
#include <getopt.h>
#include <unistd.h>
#endif

#endif /* _PORTABHACKS_H_ */
