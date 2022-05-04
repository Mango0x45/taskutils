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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NOHOME "the 'HOME' environment variable must be set"

static void entrtaskdir(void);

int
main(int argc, char **argv)
{
	int rval = EXIT_SUCCESS;

	if (argc == 1) {
		fputs("Usage: rmtask task...\n", stderr);
		rval = EXIT_FAILURE;
	} else {
		entrtaskdir();
		while (*++argv) {
			if (unlink(*argv) == -1) {
				warn("unlink: '%s'", *argv);
				rval = EXIT_FAILURE;
			}
		}
	}

	return rval;
}

void
entrtaskdir(void)
{
	char *bdir, *tdir = "tasks";

	if ((bdir = getenv("XDG_DATA_HOME")) == NULL || *bdir == '\0') {
		if ((bdir = getenv("HOME")) == NULL)
			errx(EXIT_FAILURE, NOHOME);
		tdir = ".tasks";
	}

	if (chdir(bdir) == -1)
		err(EXIT_FAILURE, "chdir: '%s'", bdir);
	if (chdir(tdir) == -1)
		err(EXIT_FAILURE, "chdir: '%s/%s'", bdir, tdir);
}
