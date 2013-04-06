#include <stdio.h>
#include <stdlib.h>

#include "decls.h"

ssize_t
getline(char** lineptr, size_t* n, FILE* stream)
{
	int c;
	size_t sz = 0;

	while (c = getc (stream), c != EOF){
		if (sz+1 >= *n){
			/* +2 is for `c' and 0-terminator */
			*n = *n * 3 / 2 + 2;
			*lineptr = realloc (*lineptr, *n);
			if (!*lineptr)
				return -1;
		}

		(*lineptr) [sz++] = (char) c;
		if (c == '\n')
			break;
	}

	if (ferror (stdin))
		return (ssize_t) -1;

	if (!sz){
		if (feof (stdin)){
			return (ssize_t) -1;
		}else if (!*n){
			*lineptr = malloc (1);
			if (!*lineptr)
				return -1;
			*n = 1;
		}
	}

	(*lineptr) [sz] = 0;
	return sz;
}
