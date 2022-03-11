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
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <task.h>

static void usage(const char *argv0);
static void setnow(struct tm *dest);
static void authoradd(struct task *tsk, char *s);
static void bodyadd(struct task *tsk);

int
main(int argc, char **argv)
{
	int err, opt;
	char *s, *st, *et;
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

	if (st != NULL) {
		if (strcmp(st, ".") == 0)
			setnow(&tsk.start);
		else if ((s = strptime(st, "%H:%M %Y-%m-%d", &tsk.start)) == NULL || *s != '\0')
			errx(EXIT_FAILURE, "Invalid datetime format");
	}
	if (et != NULL) {
		if (strcmp(et, ".") == 0)
			setnow(&tsk.end);
		else if ((s = strptime(et, "%H:%M %Y-%m-%d", &tsk.end)) == NULL || *s != '\0')
			errx(EXIT_FAILURE, "Invalid datetime format");
	}

	bodyadd(&tsk);

	if ((err = taskwrite(stdout, tsk)) != 0)
		errx(EXIT_FAILURE, "taskwrite: %s", strerror(err));

	return EXIT_SUCCESS;
}

void
setnow(struct tm *dest)
{
	time_t t;
	struct tm *src;

	if ((t = time(NULL)) == (time_t) -1)
		err(EXIT_FAILURE, "time");
	if ((src = gmtime(&t)) == NULL)
		err(EXIT_FAILURE, "gmtime");

	memcpy(dest, src, sizeof(struct tm));
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
