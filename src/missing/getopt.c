/*
** Copyright (C) 1987,88,89,90,91,92,93,94,95,96,98,99,2000,2001
** Free Software Foundation, Inc.
** This file is part of the GNU C Library.
**
** The GNU C Library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** The GNU C Library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with the GNU C Library; if not, write to the Free
** Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
** 02111-1307 USA.
**
** Code cleanup for oidentd Copyright (C) 2001-2003 Ryan McCabe <ryan@numb.org>
*/

#include <config.h>

#ifndef HAVE_GETOPT_LONG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt_missing.h>

char *optarg;
int optind = 1;
int __getopt_initialized;
static char *nextchar;
int opterr = 1;
int optopt = '?';

static enum {
	REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

static char *posixly_correct;

static char *my_index(const char *str, int chr) {
	while (*str) {
		if (*str == chr)
			return ((char *) str);
		str++;
	}

	return (0);
}

static int first_nonopt;
static int last_nonopt;

#define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0')

static void exchange(char **argv) {
	int bottom = first_nonopt;
	int middle = last_nonopt;
	int top = optind;
	char *tem;

	while (top > middle && middle > bottom) {
		if (top - middle > middle - bottom) {
			int len = middle - bottom;
			int i;

			for (i = 0; i < len; i++) {
				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
			}

			top -= len;
		} else {
			int len = top - middle;
			int i;

			for (i = 0; i < len; i++) {
				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
			}

			bottom += len;
		}
	}

	first_nonopt += (optind - last_nonopt);
	last_nonopt = optind;
}

static const char *_getopt_initialize(	int argc,
										char *const *argv,
										const char *optstring)
{
	(void) argc;
	(void) argv;

	first_nonopt = last_nonopt = optind;
	nextchar = NULL;
	posixly_correct = getenv("POSIXLY_CORRECT");

	if (optstring[0] == '-') {
		ordering = RETURN_IN_ORDER;
		++optstring;
	} else if (optstring[0] == '+') {
		ordering = REQUIRE_ORDER;
		++optstring;
	} else if (posixly_correct != NULL)
		ordering = REQUIRE_ORDER;
	else
		ordering = PERMUTE;

	return (optstring);
}

int _getopt_internal(	int argc,
						char *const *argv,
						const char *optstring,
						const struct option *longopts,
						int *longind,
						int long_only)
{
	int print_errors = opterr;

	if (optstring[0] == ':')
		print_errors = 0;

	if (argc < 1)
		return (-1);

	optarg = NULL;

	if (optind == 0 || !__getopt_initialized) {
		if (optind == 0)
			optind = 1;

		optstring = _getopt_initialize(argc, argv, optstring);
		__getopt_initialized = 1;
	}

	if (nextchar == NULL || *nextchar == '\0') {
		if (last_nonopt > optind)
			last_nonopt = optind;

		if (first_nonopt > optind)
			first_nonopt = optind;

		if (ordering == PERMUTE) {
			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char **) argv);
			else if (last_nonopt != optind)
				first_nonopt = optind;

			while (optind < argc && NONOPTION_P)
				optind++;

			last_nonopt = optind;
		}

		if (optind != argc && !strcmp(argv[optind], "--")) {
			optind++;

			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char **) argv);
			else if (first_nonopt == last_nonopt)
				first_nonopt = optind;

			last_nonopt = argc;
			optind = argc;
		}

		if (optind == argc) {
			if (first_nonopt != last_nonopt)
				optind = first_nonopt;

			return (-1);
		}

		if (NONOPTION_P) {
			if (ordering == REQUIRE_ORDER)
				return (-1);

			optarg = argv[optind++];
			return (1);
		}

		nextchar =
			(argv[optind] + 1 + (longopts != NULL && argv[optind][1] == '-'));
	}

	if (longopts != NULL
		&& (argv[optind][1] == '-'
			|| (long_only && (argv[optind][2]
			|| !my_index (optstring, argv[optind][1])))))
	{
		char *nameend;
		const struct option *p;
		const struct option *pfound = NULL;
		int exact = 0;
		int ambig = 0;
		int indfound = -1;
		int option_index;

		for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
			;

		for (p = longopts, option_index = 0; p->name; p++, option_index++) {
			if (!strncmp(p->name, nextchar, nameend - nextchar)) {
				if ((unsigned int) (nameend - nextchar) ==
					(unsigned int) strlen(p->name))
				{
					pfound = p;
					indfound = option_index;
					exact = 1;
					break;
				} else if (pfound == NULL) {
					pfound = p;
					indfound = option_index;
				} else if (long_only || pfound->has_arg != p->has_arg
							|| pfound->flag != p->flag
							|| pfound->val != p->val)
				{
					ambig = 1;
				}
			}
		}

		if (ambig && !exact) {
			if (print_errors) {
				fprintf(stderr, "%s: option `%s' is ambiguous\n",
					argv[0], argv[optind]);
			}

			nextchar += strlen(nextchar);
			optind++;
			optopt = 0;

			return ('?');
		}

		if (pfound != NULL) {
			option_index = indfound;
			optind++;

			if (*nameend) {
				if (pfound->has_arg)
					optarg = nameend + 1;
				else {
					if (print_errors) {
						if (argv[optind - 1][1] == '-') {
							fprintf(stderr,
								"%s: option `--%s' doesn't allow an argument\n",
								argv[0], pfound->name);
						} else {
							fprintf(stderr,
								"%s: option `%c%s' doesn't allow an argument\n",
								argv[0], argv[optind - 1][0], pfound->name);
						}
					}

					nextchar += strlen(nextchar);
					optopt = pfound->val;

					return ('?');
				}
			} else if (pfound->has_arg == 1) {
				if (optind < argc)
					optarg = argv[optind++];
				else {
					if (print_errors) {
						fprintf(stderr,
							"%s: option `%s' requires an argument\n",
							argv[0], argv[optind - 1]);
					}

					nextchar += strlen(nextchar);
					optopt = pfound->val;

					return (optstring[0] == ':' ? ':' : '?');
				}
			}

			nextchar += strlen(nextchar);

			if (longind != NULL)
				*longind = option_index;

			if (pfound->flag) {
				*(pfound->flag) = pfound->val;
				return (0);
			}

			return (pfound->val);
		}

		if (!long_only || argv[optind][1] == '-'
			|| my_index(optstring, *nextchar) == NULL)
		{
			if (print_errors) {
				if (argv[optind][1] == '-') {
					fprintf(stderr, "%s: unrecognized option `--%s'\n",
						argv[0], nextchar);
				} else {
					fprintf(stderr, "%s: unrecognized option `%c%s'\n",
						argv[0], argv[optind][0], nextchar);
				}
			}

			nextchar = (char *) "";
			optind++;
			optopt = 0;

			return ('?');
		}
	}

	{
		char c = *nextchar++;
		char *temp = my_index(optstring, c);

		if (*nextchar == '\0')
			++optind;

		if (temp == NULL || c == ':') {
			if (print_errors) {
				if (posixly_correct) {
					fprintf(stderr, "%s: illegal option -- %c\n", argv[0], c);
				} else {
					fprintf (stderr, "%s: invalid option -- %c\n", argv[0], c);
				}
			}

			optopt = c;
			return ('?');
		}

		if (temp[0] == 'W' && temp[1] == ';') {
			char *nameend;
			const struct option *p;
			const struct option *pfound = NULL;
			int exact = 0;
			int ambig = 0;
			int indfound = 0;
			int option_index;

			if (*nextchar != '\0') {
				optarg = nextchar;
				optind++;
			} else if (optind == argc) {
				if (print_errors) {
					fprintf(stderr, "%s: option requires an argument -- %c\n",
								argv[0], c);
				}

				optopt = c;
				if (optstring[0] == ':')
					c = ':';
				else
					c = '?';

				return (c);
			} else
				optarg = argv[optind++];

				for (nextchar = nameend = optarg ;
					*nameend && *nameend != '=' ; nameend++)
				{
					;
				}

				for (p = longopts, option_index = 0 ;
					p->name; p++, option_index++)
				{
					if (!strncmp(p->name, nextchar, nameend - nextchar)) {
						if ((unsigned int) (nameend - nextchar)
							== strlen(p->name))
						{
							pfound = p;
							indfound = option_index;
							exact = 1;
							break;
						} else if (pfound == NULL) {
							pfound = p;
							indfound = option_index;
						} else
							ambig = 1;
					}
				}

			if (ambig && !exact) {
				if (print_errors) {
					fprintf(stderr, "%s: option `-W %s' is ambiguous\n",
							argv[0], argv[optind]);
				}

				nextchar += strlen(nextchar);
				optind++;

				return ('?');
			}

			if (pfound != NULL) {
				option_index = indfound;

				if (*nameend) {
					if (pfound->has_arg)
						optarg = nameend + 1;
					else {
						if (print_errors) {
							fprintf(stderr,
								"%s option `-W %s' doesn't allow an argument\n",
								argv[0], pfound->name);
						}

						nextchar += strlen(nextchar);
						return ('?');
					}
				} else if (pfound->has_arg == 1) {
					if (optind < argc)
						optarg = argv[optind++];
					else {
						if (print_errors) {
							fprintf(stderr,
								"%s: option `%s' requires an argument\n",
								argv[0], argv[optind - 1]);
						}

						nextchar += strlen(nextchar);
						return (optstring[0] == ':' ? ':' : '?');
					}
				}

				nextchar += strlen(nextchar);
				if (longind != NULL)
					*longind = option_index;

				if (pfound->flag) {
					*(pfound->flag) = pfound->val;
					return (0);
				}

				return (pfound->val);
			}

			nextchar = NULL;
			return ('W');
		}

		if (temp[1] == ':') {
			if (temp[2] == ':') {
				if (*nextchar != '\0') {
					optarg = nextchar;
					optind++;
				} else
					optarg = NULL;

				nextchar = NULL;
			} else {
				if (*nextchar != '\0') {
					optarg = nextchar;
					optind++;
				} else if (optind == argc) {
					if (print_errors) {
						fprintf(stderr,
							"%s: option requires an argument -- %c\n",
							argv[0], c);
					}

					optopt = c;

					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				} else
					optarg = argv[optind++];

				nextchar = NULL;
			}
		}

		return (c);
	}
}

int getopt(int argc, char *const *argv, const char *optstring) {
	return	(_getopt_internal(argc, argv, optstring, NULL, NULL, 0));
}

int getopt_long(int argc,
				char *const *argv,
				const char *options,
				const struct option *long_options,
				int *opt_index)
{
	return (_getopt_internal(argc, argv, options, long_options, opt_index, 0));
}

int getopt_long_only(	int argc,
						char *const *argv,
						const char *options,
						const struct option *long_options,
						int *opt_index)
{
	return (_getopt_internal(argc, argv, options, long_options, opt_index, 1));
}

#endif
