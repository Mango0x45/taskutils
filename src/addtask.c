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

#ifndef O_PATH
	#define O_PATH O_RDONLY
#endif
#ifndef DEFFILEMODE
	/* TODO: Check if this macro is actually standardized anywhere. */
	#define DEFFILEMODE 0666
#endif

#define NOHOME "the 'HOME' environment variable must be set"

void mktaskdir(void);
int dirsize(void);
void taskadd(int fd, int id);

int rval;

int
main(int argc, char **argv)
{
	int rfd, ffd, fcnt;

	if ((rfd = open(".", O_PATH)) == -1)
		err(EXIT_FAILURE, "open: '.'");

	mktaskdir();
	fcnt = dirsize();

	if (argc == 1)
		taskadd(STDIN_FILENO, fcnt++);
	else while (*++argv) {
		if (strcmp(*argv, "-") == 0)
			taskadd(STDIN_FILENO, fcnt++);
		else if ((ffd = openat(rfd, *argv, O_RDONLY)) == -1) {
			warn("openat: '%s'", *argv);
			rval = EXIT_FAILURE;
		} else {
			taskadd(ffd, fcnt++);
			close(ffd);
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

int
dirsize(void)
{
	int cnt = 0;
	DIR *dirp;
	struct dirent *e;

	if ((dirp = opendir(".")) == NULL)
		err(EXIT_FAILURE, "opendir: '%s'", realpath(".", NULL));

	while ((e = readdir(dirp)) != NULL) {
		if (e->d_type == DT_REG)
			cnt++;
	}

	closedir(dirp);
	return cnt;
}

void
taskadd(int ifd, int id)
{
	int ofd, nr;
	char fname[9], buf[BUFSIZ];

	sprintf(fname, "%08d", id);
	fname[8] = '\0';

	if ((ofd = creat(fname, DEFFILEMODE)) == -1)
		err(EXIT_FAILURE, "creat: '%s'", fname);

	while ((nr = read(ifd, buf, BUFSIZ)) > 0) {
		if (write(ofd, buf, nr) == -1)
			err(EXIT_FAILURE, "write: '%s'", fname);
	}
	if (nr == -1)
		err(EXIT_FAILURE, "read: '%s'", fname);

	close(ofd);
}
