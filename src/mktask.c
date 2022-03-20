/*
 * BSD Zero Clause License
 *
 * Copyright (c) 2022 Thomas Voss
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define _XOPEN_SOURCE
#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <task.h>

static void authoradd(struct task *tsk, char *s);
static void bodyadd(struct task *tsk);
static void setnoweoff(struct tm *dest, time_t off);
static void setnowoff(struct tm *dest, time_t off);
static void setnowsoff(struct tm *dest, time_t off);
static void stimeparse(struct tm *dest, char *s);
static void timeparse(struct tm *dest, char *s);
static void usage(const char *argv0);

const char *badstimemod =
	"invalid short datetime modifier '%s', only '^' and '$' can be used";
const char *badstimestart =
	"short datetime must start with either '.' or a decimal integer";

int
main(int argc, char **argv)
{
	int err, opt;
	char *st, *et;
	bool aflag, oflag, uflag;
	struct task tsk = {0};

	st = et = NULL;
	aflag = oflag = uflag = false;

	while ((opt = getopt(argc, argv, ":a:o:u:")) != -1) {
		switch (opt) {
		case 'a':
			if (aflag || oflag)
				usage(argv[0]);
			st = optarg;
			aflag = true;
			break;
		case 'o':
			if (aflag || oflag || uflag)
				usage(argv[0]);
			st = et = optarg;
			oflag = true;
			break;
		case 'u':
			if (oflag || uflag)
				usage(argv[0]);
			et = optarg;
			uflag = true;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (!(aflag || oflag || uflag) || (argc - optind == 0))
		usage(argv[0]);

	argv += optind;
	tsk.title = *argv;
	while (*++argv)
		authoradd(&tsk, *argv);

	/* Set the start and end times if we are using them.  The `timeparse'
	 * function will take the string representation of the time and convert
	 * it into a `struct tm'. */
	if (st != NULL)
		timeparse(&tsk.start, st);
	if (et != NULL)
		timeparse(&tsk.end, et);

	bodyadd(&tsk);

	if ((err = taskwrite(stdout, tsk)) != 0)
		errx(EXIT_FAILURE, "taskwrite: %s", strerror(err));

	return EXIT_SUCCESS;
}

void
timeparse(struct tm *dest, char *s)
{
	char *r;

	if ((r = strptime(s, "%H:%M %Y-%m-%d", dest)) == NULL || *r != '\0')
		stimeparse(dest, s);
}

void
stimeparse(struct tm *dest, char *s)
{
	long off;
	char *ptr;

	if (s[0] == '.') {
		if (s[1] == '\0')
			setnowoff(dest, 0);
		else if (s[1] == '^' && s[2] == '\0')
			setnowsoff(dest, 0);
		else if (s[1] == '$' && s[2] == '\0')
			setnoweoff(dest, 0);
		else
			errx(EXIT_FAILURE, badstimemod, s + 1);
	} else if (s[0] == '+' || s[0] == '-' || isdigit(s[0])) {
		off = strtol(s, &ptr, 10);
		if (*ptr == '\0')
			setnowoff(dest, off * 86400);
		else if (*ptr == '^' && *(ptr + 1) == '\0')
			setnowsoff(dest, off * 86400);
		else if (*ptr == '$' && *(ptr + 1) == '\0')
			setnoweoff(dest, off * 86400);
		else
			errx(EXIT_FAILURE, badstimemod, s);
	} else
		errx(EXIT_FAILURE, badstimestart, s);
}

void
setnowoff(struct tm *dest, time_t off)
{
	time_t t;
	struct tm *src;

	if ((t = time(NULL)) == (time_t) -1)
		err(EXIT_FAILURE, "time");
	t += off;
	if ((src = gmtime(&t)) == NULL)
		err(EXIT_FAILURE, "gmtime");

	memcpy(dest, src, sizeof(struct tm));
}

void
setnowsoff(struct tm *dest, time_t off)
{
	setnowoff(dest, off);
	dest->tm_min = 0;
	dest->tm_hour = 0;
}

void
setnoweoff(struct tm *dest, time_t off)
{
	setnowoff(dest, off);
	dest->tm_min = 59;
	dest->tm_hour = 23;
}

void
authoradd(struct task *tsk, char *s)
{
	static int cap = 0;

	if (tsk->authors == NULL) {
		if ((tsk->authors = malloc(sizeof(char *) * 8)) == NULL)
			err(EXIT_FAILURE, "malloc");
		cap = 7;
	} else if (tsk->author_cnt == cap) {
		cap *= 2;
		if ((tsk->authors = realloc(tsk->authors, sizeof(char *) * cap + 1)) == NULL)
			err(EXIT_FAILURE, "realloc");
	}

	tsk->authors[tsk->author_cnt++] = s;
	tsk->authors[tsk->author_cnt] = NULL;
}

void
bodyadd(struct task *tsk)
{
	int nr, cap;
	char buf[BUFSIZ + 1];

	cap = BUFSIZ;
	if ((tsk->body = calloc(cap + 1, sizeof(char))) == NULL)
		err(EXIT_FAILURE, "calloc");

	while ((nr = read(STDIN_FILENO, buf, BUFSIZ)) > 0) {
		if (tsk->body_len + nr >= cap) {
			cap *= 2;
			if ((tsk->body = realloc(tsk->body, cap + 1)) == NULL)
				err(EXIT_FAILURE, "realloc");
		}

		buf[nr] = '\0';
		strcat(tsk->body, buf);
		tsk->body_len += nr;
	}

	if (nr == -1)
		err(EXIT_FAILURE, "read");
	if (tsk->body_len == 0) {
		free(tsk->body);
		tsk->body = NULL;
	}
}

void
usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s -a|-o|-u time title [authors...]\n"
			"       %s -a time -u time title [authors...]\n", argv0, argv0);
	exit(EXIT_FAILURE);
}
