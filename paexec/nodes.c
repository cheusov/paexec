/*
 * Copyright (c) 2007-2013 Aleksey Cheusov <vle@gmx.net>
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

#include "nodes.h"
#include "wrappers.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char **nodes    = NULL;
int nodes_count = 0;

static void nodes_create__count(const char *nodes_str)
{
	int i;

	nodes_count = (int) strtol(nodes_str, NULL, 10);
	if (nodes_count == (int) LONG_MAX)
		err__fatal_errno(NULL, "paexec: invalid option -n:");

	nodes = xmalloc(nodes_count * sizeof(nodes [0]));

	for (i=0; i < nodes_count; ++i){
		char num [50];
		snprintf(num, sizeof(num), "%d", i);
		nodes [i] = xstrdup(num);
	}
}

static void nodes_create__list(char *nodes_str)
{
	char *last = NULL;
	char *p = nodes_str;
	char c;

	/* "node1 nodes2 ..." format */
	for (;;){
		c = *p;

		switch (c){
			case ' ':
			case '\t':
			case '\r':
			case '\n':
			case 0:
				if (last){
					*p = 0;

					++nodes_count;

					nodes = xrealloc(
						nodes,
						nodes_count * sizeof(*nodes));

					nodes [nodes_count - 1] = xstrdup(last);

					last = NULL;
				}
				break;
			default:
				if (!last){
					last = p;
				}
				break;
		}

		if (!c)
			break;

		++p;
	}
}

static void nodes_create__file(const char *nodes_str)
{
	char node [4096];
	FILE *fd = fopen(nodes_str, "r");
	size_t len = 0;

	if (!fd)
		err__fatal_errno(NULL, "paexec: Cannot obtain a list of nodes");

	while (fgets(node, sizeof(node), fd)){
		len = strlen(node);
		if (len > 0 && node [len-1] == '\n')
			node [len-1] = 0;

		++nodes_count;

		nodes = xrealloc(
			nodes,
			nodes_count * sizeof(*nodes));

		nodes [nodes_count - 1] = xstrdup(node);
	}

	fclose(fd);
}

void nodes_create(const char *nodes_str)
{
	unsigned char c0 = nodes_str [0];
	char *nodes_copy;

	if (c0 == '+'){
		/* "+NUM" format */
		nodes_create__count(nodes_str + 1);
	}else if (c0 == ':'){
		/* "+NUM" format */
		nodes_create__file(nodes_str + 1);
	}else if (isalnum(c0) || c0 == '/' || c0 == '_'){
		/* list of nodes */
		nodes_copy = xstrdup(nodes_str);
		nodes_create__list(nodes_copy);
		xfree(nodes_copy);
	}else{
		err__fatal(NULL, "paexec: invalid argument for option -n");
	}

	/* final check */
	if (nodes_count == 0)
		err__fatal(NULL, "paexec: invalid argument for option -n");
}

void nodes_destroy(void)
{
	int i;

	if (nodes){
		for (i=0; i < nodes_count; ++i){
			if (nodes [i])
				xfree(nodes [i]);
		}
		xfree(nodes);
	}
}
