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

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <task.h>

#ifndef O_PATH
	#define O_PATH O_RDONLY
#endif
#ifndef DEFFILEMODE
	/* TODO: Check if this macro is actually standardized anywhere. */
	#define DEFFILEMODE 0666
#endif

#define NOHOME "the 'HOME' environment variable must be set"

void mktaskdir(void);
void taskadd(FILE *instream);

int rval;

int
main(int argc, char **argv)
{
	int rfd, ffd;
	FILE *stream;

	if ((rfd = open(".", O_PATH)) == -1)
		err(EXIT_FAILURE, "open: '.'");

	mktaskdir();

	if (argc == 1)
		taskadd(stdin);
	else while (*++argv) {
		if (strcmp(*argv, "-") == 0)
			taskadd(stdin);
		else if ((ffd = openat(rfd, *argv, O_RDONLY)) == -1) {
			warn("openat: '%s'", *argv);
			rval = EXIT_FAILURE;
		} else {
			if ((stream = fdopen(ffd, "r")) == NULL) {
				warn("fdopen: '%s'", *argv);
				rval = EXIT_FAILURE;
			}
			taskadd(stream);
			fclose(stream);
		}
	}

	close(rfd);
	return rval;
}

void
mktaskdir(void)
{
	char *bdir, *tdir = "tasks";

	if ((bdir = getenv("XDG_DATA_HOME")) == NULL || *bdir == '\0') {
		if ((bdir = getenv("HOME")) == NULL)
			errx(EXIT_FAILURE, NOHOME);
		tdir = ".tasks";
	}

	if (chdir(bdir) == -1)
		err(EXIT_FAILURE, "chdir: '%s'", bdir);
	if (mkdir(tdir, 0777) == -1 && errno != EEXIST)
		err(EXIT_FAILURE, "mkdir: '%s'", tdir);
	if (chdir(tdir) == -1)
		err(EXIT_FAILURE, "chdir: '%s'", tdir);
}

void
taskadd(FILE *instream)
{
	FILE *outstream;
	struct task tsk;

	taskread(instream, &tsk);

	outstream = fopen(tsk.title, "r");
	if (outstream == NULL && errno == ENOENT) {
		if ((outstream = fopen(tsk.title, "w")) == NULL)
			goto err_cant_open;
		taskwrite(outstream, tsk);
		fclose(outstream);
	} else if (outstream == NULL) {
err_cant_open:
		warn("fopen: '%s'", tsk.title);
		rval = EXIT_FAILURE;
	} else {
		warnx("task with name '%s' already exists", tsk.title);
		rval = EXIT_FAILURE;
	}

	taskfree(tsk);
}
