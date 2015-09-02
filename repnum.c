/*

Copyright (C) 2015  Amr Ayman

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>

#ifdef __GNUC__
#define _GNU_SOURCE
#include <getopt.h>
#elif ! (_POSIX_C_SOURCE >= 2 || _XOPEN_SOURCE)
#include <unistd.h>
#else
/* No command line options would be available at all */
#define NO_OPTS
#endif

#define PROG_VERSION (1.0f)
#define PROG_NAME "repnum"

#define iseven(n) ((n) % 2 == 0)

#define _reporterror(c, ...) \
	err(c, __VA_ARGS__)
#define _throwerror(c, ...) \
	errx(c, __VA_ARGS__)

struct numinfo {
	unsigned long long num;
	int base;
};

enum errorcodes {
	SUCCESS = 0,
	FAILURE,
	INVALID_ARGS,
	INCOMPLETE,
	OVERFLOW
};

struct numinfo info;

/*
 * Converts a number to a string containing its binary representation.
 * `num` is the number to convert.
 * `buf` is the string buffer to use.
 * `n` is the length of the string.
 * Returns a pointer to the start of the representation. NOT THE START OF `buf`.
 * If NULL was returned, then `n` characters are not enough and the function stopped at the middle of processing.
 * NOTE: the algorithm used writes binary right to left, so to ensure highest performance,
 * characters are written starting by the end of the string.
 */
char *num2bin(unsigned long long num, char *buf, size_t n);

/*
 * Converts a string to a long.
 * Converts `str` as a base `base` number to long and stores it in `num`.
 * Supports all bases that strtoull supports.
 * Returns:
 *	SUCCESS - Whole `str` is valid. `num` is changed.
 *	FAILURE - Whole `str` is invalid. `num` is not changed.
 *	INVALID_ARGS - `num` is null or `str` is null or the first character in `str` is a null byte. `num` is not changed.
 *	INCOMPLETE - Partial `str` is valid. `num` is changed.
 *	OVERFLOW - Overflow. `num` is not changed.
 */
enum errorcodes str2ull(char *str, unsigned long long *num, int base);

/*					Convenience Functions						*/

/*
 * Note: `num` may be null, in which case a temporary variable is used and returned instead.
 */
static inline unsigned long long ensurenum(char *str, unsigned long long *num, int base);
static inline int ensurebase(int);
static inline void help(int);

#ifndef NO_OPTS
static void parseopts(int argc, char **argv);
#endif

int main(int argc, char **argv)
{
	char *res;
	char buffer[1024];

	info.base = 0;
	parseopts(argc, argv);

	if (!(res = num2bin(info.num, buffer, sizeof(buffer))))
		_throwerror(1, "%s is a too large number.", argv[1]);

	printf("[dec]\t%llu\t=\t[hex]\t%llx\t[oct]\t%llo\t[bin]\t%s\n", info.num, info.num, info.num, res);

	return 0;
}

char *num2bin(unsigned long long num, char *buf, size_t n)
{
	char s;

	buf[n-1] = 0;

	while (--n) {
		s = iseven(num) ? '0' : '1';
		buf[n-1] = s;
		if (!(num /= 2))
			break;
	}

	return &buf[n-1];
}

enum errorcodes str2ull(char *str, unsigned long long *num, int base)
{
	char *end;
	unsigned long long status;

	if (!str || !*str || !num)
		return INVALID_ARGS;

	if(!(status = strtoull(str, &end, base)) && (end == str))
		return FAILURE;

	if (errno == ERANGE)
		return OVERFLOW;

	*num = status;

	return (*end ? INCOMPLETE : SUCCESS);
}

static inline void help(int c)
{
	puts("Displays a number in various base representations.\n\t-b"
#ifdef _GNU_SOURCE
		"|--base"
#else
		"\t"
#endif
		" [base]\tforce a base. Possible values are 2 through 36.\n\t-v"
#ifdef _GNU_SOURCE
		"|--version"
#endif
		"\t\toutput version then exit.\n\t"
#ifdef _GNU_SOURCE
		"--help\t"
#else
		"-h"
#endif
		"\t\tview this help.");
	exit(c);
}

#ifndef NO_OPTS
static void parseopts(int argc, char **argv)
{
	int opt;

#ifdef _GNU_SOURCE
	struct option opts[] = {
		{ "base", 1, 0, 'b' },
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 'v' }
	};
#define parser() (getopt_long(argc, argv, "b:hv", opts, 0))
#else
#define parser() (getopt(argc, argv, "b:hv"))
#endif
	while ((opt = parser()) != -1) {
		switch (opt)
		{
			case '?':
				_throwerror(1, "Unrecognized option: -%c", opt);
			case 'b':
				info.base = ensurebase(ensurenum(optarg, 0, 10));
				break;
			case 'h':
				help(0);
			case 'v':
				printf("%s v%.2f. Licenced under the GNU GPL v3 License.\n", PROG_NAME, PROG_VERSION);
				exit(0);
		}
	}

	if (optind >= argc)
		help(1);
	info.num = ensurenum(argv[optind], 0, info.base);
}
#endif

/*								Convenience Functions							*/

static inline unsigned long long ensurenum(char *s, unsigned long long *n, int b)
{
	unsigned long long temp;
	enum errorcodes status;
	char tmp[25];

	/* if the user doesn't provide his variable two change, create a temporary one */
	if (!n)
		n = &temp;
	
	status = str2ull(s, n, b);

	/* generate a message to indicate required base if `b` is not 0 [any base]. */
	if (b)
		snprintf(tmp, 25, "Base %d is required.", b);

	switch (status)
	{
		case FAILURE:
		case INCOMPLETE:
			_throwerror(1, "%s is not a valid number.%s", s, b ? tmp : "");
		case OVERFLOW:
			_throwerror(1, "%s is a too large number.", s);
	}

	return *n;
}

static inline int ensurebase(int b)
{
	if (b < 2 || b > 36)
		_throwerror(1, "Unsupported base: %d", b);
	return b;
}
