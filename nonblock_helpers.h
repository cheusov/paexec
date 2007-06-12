#ifndef _NONBLOCK_HELPERS_H_
#define _NONBLOCK_HELPERS_H_

typedef void (*line_putter_t) (const char *, void *);

/* returns 1 if eof happens */
int put_until_emptyline (int fd, line_putter_t putter, void *data);

#endif // _NONBLOCK_HELPERS_H_
