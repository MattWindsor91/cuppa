/*
 * io.c - input/output
 *   Part of cuppa, the Common URY Playout Package Architecture
 *
 * Contributors:  Matt Windsor <matt.windsor@ury.org.uk>
 */

/*-
 * Copyright (c) 2012, University Radio York Computing Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _POSIX_C_SOURCE 200809

#include <stdarg.h>		/* print functions */
#include <stdbool.h>		/* booleans */
#include <stdio.h>		/* printf, fprintf */
#include <time.h>		/* select etc. */
#include <unistd.h>

#include <sys/select.h>		/* select */

#include "constants.h"		/* WORD_LEN */
#include "io.h"			/* enum response */

/* Names of responses.
 * Names should always just be the constant with leading R_ removed.
 */
const char     RESPONSES[NUM_RESPONSES][WORD_LEN] = {
	/* 'Pull' responses (initiated by client command) */
	"OKAY",			/* R_OKAY */
	"WHAT",			/* R_WHAT */
	"FAIL",			/* R_FAIL */
	"OOPS",			/* R_OOPS */
	/* 'Push' responses (initiated by server) */
	"OHAI",			/* R_OHAI */
	"TTFN",			/* R_TTFN */
	"STAT",			/* R_STAT */
	"TIME",			/* R_TIME */
	"DBUG",			/* R_DBUG */
};

/* Whether or not responses should be sent to stdout (to the client). */
bool		RESPONSE_STDOUT[NUM_RESPONSES] = {
	/* 'Pull' responses (initiated by client command) */
	true,			/* R_OKAY */
	true,			/* R_WHAT - usually not a real error per se */
	true,			/* R_FAIL */
	true,			/* R_OOPS */
	/* 'Push' responses (initiated by server) */
	true,			/* R_OHAI */
	true,			/* R_TTFN */
	true,			/* R_STAT */
	true,			/* R_TIME */
	false,			/* R_DBUG - should come up in logs etc. */
};

/* Whether or not responses should be sent to stderr (usually logs/console). */
bool		RESPONSE_STDERR[NUM_RESPONSES] = {
	/* 'Pull' responses (initiated by client command) */
	false,			/* R_OKAY */
	false,			/* R_WHAT - usually not a real error per se */
	true,			/* R_FAIL */
	true,			/* R_OOPS */
	/* 'Push' responses (initiated by server) */
	false,			/* R_OHAI */
	false,			/* R_TTFN */
	false,			/* R_STAT */
	false,			/* R_TIME */
	true,			/* R_DBUG - should come up in logs etc. */
};

/* Sends a response to standard out and, for certain responses, standard error.
 * This is the base function for all system responses.
 */
enum response
vresponse(enum response code, const char *format, va_list ap)
{
	va_list ap2;

	va_copy(ap2, ap);

	if (RESPONSE_STDOUT[(int)code]) {
		printf("%s ", RESPONSES[(int)code]);
		vprintf(format, ap);
		printf("\n");
	}

	if (RESPONSE_STDERR[(int)code]) {
		fprintf(stderr, "%s ", RESPONSES[(int)code]);
		vfprintf(stderr, format, ap2);
		fprintf(stderr, "\n");
	}

	return code;
}

/* Sends a response to standard out and, for certain responses, standard error.
 * This is a wrapper around 'vresponse'.
 */
enum response
response(enum response code, const char *format,...)
{
	va_list		ap;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	vresponse(code, format, ap);
	va_end(ap);

	return code;
}

/* Returns true if input is waiting on standard in. */
int
input_waiting(void)
{
	fd_set		rfds;
	struct timeval	tv;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	/* Stop checking immediately. */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(1, &rfds, NULL, NULL, &tv);
}
