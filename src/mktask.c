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

#define _XOPEN_SOURCE 500
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
			/* The -a flag can be used with the -u flag but not with
			 * the -o flag, and sets the start time. */
			if (aflag || oflag)
				usage(argv[0]);
			st = optarg;
			aflag = true;
			break;
		case 'o':
			/* The -o flag cannot be used with any other flag, and
			 * sets the start and end time to the same thing */
			if (aflag || oflag || uflag)
				usage(argv[0]);
			st = et = optarg;
			oflag = true;
			break;
		case 'u':
			/* The -u flag can be used with the -a flag but not with
			 * the -o flag, and sets the end time. */
			if (oflag || uflag)
				usage(argv[0]);
			et = optarg;
			uflag = true;
			break;
		default:
			usage(argv[0]);
		}
	}

	/* A correct usage of the program uses at least one flag, and has a
	 * title provided as a command line argument. */
	if (!(aflag || oflag || uflag) || (argc - optind == 0))
		usage(argv[0]);

	/* Set the title of the task, and then add all of the remaining command
	 * line arguments to the task as authors.  Here we have a debug and non
	 * debug version where the debug version allocates a new string with
	 * `strdup()'.  This is done so that `taskfree()' doesn't try to free
	 * memory that wasn't dynamically allocated. */
	argv += optind;
#ifndef DEBUG
	tsk.title = *argv;
	while (*++argv)
		authoradd(&tsk, *argv);
#else
	tsk.title = strdup(*argv);
	while (*++argv)
		authoradd(&tsk, strdup(*argv));
#endif

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

#ifdef DEBUG
	/* We can just not include this code at all for better performance, but
	 * we want to free the task when debugging to more easily find bugs with
	 * valgrind or whatever. */
	taskfree(tsk);
#endif

	return EXIT_SUCCESS;
}

/* Try to parse the time string `s' as a long time string.  If this fails,
 * fallback on trying to parse it as a short time. */
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

	/* Get the current time with `time' which is returned as a `time_t'.
	 * Then pass the time to `gmtime' which returns a pointer to a `struct
	 * tm' which is the format we want.  It should be noted that the
	 * returned pointer is to a static struct, not a dynamically allocated
	 * one, so no need to worry about extra allocations.  Before we pass the
	 * current time to `gmtime' we want to offset it by `off'. */
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

/* Add the author specified by the string `s' to the task specified by the
 * struct `tsk'.  If the structs `authors' array is NULL (so it hasn't been
 * alocated yet) then it gets allocated dynamically. */
void
authoradd(struct task *tsk, char *s)
{
	static int cap = 0;

	/* If the `authors' array is NULL, then allocate a buffer for 7 authors.
	 * This should be enough for almost every use case.  There is
	 * technically space for 8 authors, but we want to NULL terminate the
	 * array.  If we do need space for more authors we double the capacity.
	 */
	if (tsk->authors == NULL) {
		if ((tsk->authors = malloc(sizeof(char *) * 8)) == NULL)
			err(EXIT_FAILURE, "malloc");
		cap = 7;
	} else if (tsk->author_cnt == cap) {
		cap *= 2;
		if ((tsk->authors = realloc(tsk->authors,
					    sizeof(char *) * cap + 1)) == NULL)
			err(EXIT_FAILURE, "realloc");
	}

	/* Append the author to the end of the `authors' array and increment the
	 * author count. Then NULL terminate the array. */
	tsk->authors[tsk->author_cnt++] = s;
	tsk->authors[tsk->author_cnt] = NULL;
}

/* Read text from standard input and write it to the `body' field of the input
 * struct `task'.  Buffers of size BUFSIZ are used as I believe that is the
 * optimal I/O buffer size.  Please correct me if I am wrong.  When this
 * function is called, `tsk->body' will be NULL.  This function will dynamically
 * allocate a buffer for it. */
void
bodyadd(struct task *tsk)
{
	int nr, cap;
	char buf[BUFSIZ + 1];

	/* Allocate an initial buffer for the body.  It is important to use
	 * `calloc' instead of `malloc' as we want to zero the buffer.  This is
	 * because we read data into the buffer by using `strcat', which needs
	 * NUL bytes to work properly. */
	cap = BUFSIZ;
	if ((tsk->body = calloc(cap + 1, sizeof(char))) == NULL)
		err(EXIT_FAILURE, "calloc");

	/* Read `BUFSIZ' bytes from the standard input into the buffer `buf'.
	 * If the amount of bytes read would overflow `tsk->body' then double
	 * the buffers capacity.  Once the buffer size is dealt with then NUL
	 * terminate `buf' and concatinate it to the body. */
	while ((nr = read(STDIN_FILENO, buf, BUFSIZ)) > 0) {
		if (tsk->body_len + nr >= cap) {
			cap *= 2;
			if ((tsk->body = realloc(tsk->body, cap + 1)) == NULL)
				err(EXIT_FAILURE, "realloc");
		}

		/* TODO: Replace `strcat' with `memcpy'? */
		buf[nr] = '\0';
		strcat(tsk->body, buf);
		tsk->body_len += nr;
	}

	/* If `nr' is -1 then there was a failure with the call to `read'.
	 * Additionally, if `tsk->body_len' is 0 it means we didn't actually
	 * read any bytes into the body.  When this happens we want to free it
	 * and set it to NULL, because `taskwrite' will use whether or not the
	 * body is NULL to see if the task has a body or not. */
	if (nr == -1)
		err(EXIT_FAILURE, "read");
	if (tsk->body_len == 0) {
		free(tsk->body);
		tsk->body = NULL;
	}
}

/* Print a usage message to the standard error and exit the program.  This is
 * only called when someone used the program incorrectly so there is no reason
 * why you wouldn't exit with `EXIT_FAILURE'. */
void
usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s -a|-o|-u time title [authors...]\n"
			"       %s -a time -u time title [authors...]\n", argv0, argv0);
	exit(EXIT_FAILURE);
}
