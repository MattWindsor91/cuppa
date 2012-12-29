/*
 * errors.c - error reporting
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

#include <stdarg.h>		/* va_list etc. */
#include <stdio.h>		/* snprintf */
#include <stdlib.h>		/* calloc */

#include "errors.h"		/* enum error, enum error_blame */
#include "io.h"			/* vresponse, enum response */
#include "messages.h"		/* MSG_ERR_NOMEM */

/* Names for each error in the system.
 * Names should always just be the constant with leading E_ removed.
 * (More human-friendly information should be in the error string.)
 */
const char     *ERRORS[NUM_ERRORS] = {
	"OK",			/* E_OK */
	/* User errors */
	"NO_FILE",		/* E_NO_FILE */
	"BAD_STATE",		/* E_BAD_STATE */
	"BAD_COMMAND",		/* E_BAD_COMMAND */
	/* Environment errors */
	"BAD_FILE",		/* E_BAD_FILE */
	"BAD_CONFIG",		/* E_BAD_CONFIG */
	/* System errors */
	"AUDIO_INIT_FAIL",	/* E_AUDIO_INIT_FAIL */
	"INTERNAL_ERROR",	/* E_INTERNAL_ERROR */
	"NO_MEM",		/* E_NO_MEM */
	/* Misc */
	"EOF",			/* E_EOF */
	"UNKNOWN",		/* E_UNKNOWN */
};

/* Mappings of errors to the factor that we assign blame to.
 * This is used to decide which sort of response to send the error as.
 */
const enum error_blame ERROR_BLAME[NUM_ERRORS] = {
	EB_PROGRAMMER,		/* E_OK - should never raise an error message */
	/* User errors */
	EB_USER,		/* E_NO_FILE */
	EB_USER,		/* E_BAD_STATE */
	EB_USER,		/* E_BAD_COMMAND */
	/* Environment errors */
	EB_ENVIRONMENT,		/* E_BAD_FILE */
	EB_ENVIRONMENT,		/* E_BAD_CONFIG */
	/* System errors */
	EB_ENVIRONMENT,		/* E_AUDIO_INIT_FAIL */
	EB_PROGRAMMER,		/* E_INTERNAL_ERROR */
	EB_ENVIRONMENT,		/* E_NO_MEM */
	/* Misc */
	EB_PROGRAMMER,		/* E_EOF - should never show an error message */
	EB_PROGRAMMER,		/* E_UNKNOWN - error should be more specific */
};

/* This maps error blame factors to response codes. */
const enum response BLAME_RESPONSE[NUM_ERROR_BLAMES] = {
	R_WHAT,			/* EB_USER */
	R_FAIL,			/* EB_ENVIRONMENT */
	R_OOPS,			/* EB_PROGRAMMER */
};

/* Sends a debug message. */
void
dbug(const char *format,...)
{
	va_list		ap;

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	vresponse(R_DBUG, format, ap);
	va_end(ap);
}

/* Throws an error message.
 *
 * This does not propagate the error anywhere - the error code will need to be
 * sent up the control chain and handled at the top of the player.  It merely
 * sends a response through stdout and potentially stderr to let the client/logs
 * know something went wrong.
 */
enum error
error(enum error code, const char *format,...)
{
	va_list		ap;	/* Variadic arguments */
	va_list		ap2;	/* Variadic arguments */
	const char     *emsg;	/* Stores error name */
	char           *buf;	/* Temporary buffer for rendering error */
	size_t		buflen;	/* Length for creating buffer */

	buf = NULL;
	emsg = ERRORS[(int)code];

	/* LINTED lint doesn't seem to like va_start */
	va_start(ap, format);
	va_copy(ap2, ap);

	/*
	 * Problem: adding the error name into the format without changing
	 * the format string dynamically (a bad idea).
	 *
	 * Our strategy: render the error into a temporary buffer using magicks,
	 * then send it to response.
	 */

	/*
	 * Try printing the error into a null buffer to get required length
	 * (see http://stackoverflow.com/questions/4899221)
	 */
	buflen = vsnprintf(NULL, 0, format, ap);
	buf = calloc(buflen + 1, sizeof(char));
	if (buf != NULL)
		vsnprintf(buf, buflen + 1, format, ap2);

	response(BLAME_RESPONSE[ERROR_BLAME[code]], "%s %s",
		 emsg,
		 buf == NULL ? MSG_ERR_NOMEM : buf);
	va_end(ap);

	if (buf != NULL)
		free(buf);

	return code;
}
