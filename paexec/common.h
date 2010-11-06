#ifndef _COMMON_H_
#define _COMMON_H_

#if defined(__GNUC__)
#define attr_unused __attribute__ ((unused))
#else
#define attr_unused
#endif

#endif /* _COMMON_H_ */
