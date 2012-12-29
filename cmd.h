/*
 * cmd.h - command parser
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

#ifndef CUPPA_CMD_H
#define CUPPA_CMD_H

#include "constants.h"		/* WORD_LEN */
#include "errors.h"		/* enum error */

/*
 * Any code defining a set of commands SHOULD use these macros and
 * MUST terminate with END_CMDS or an equivalent.
 *
 * Example:
 *
 * struct cmd *foo = {
 *   NCMD("acme", command_with_no_argument),
 *   UCMD("ecma", command_with_an_argument),
 *   END_CMDS
 * }
 */
#define NCMD(word, func) {word, C_NULLARY, {.ncmd = func}}
#define UCMD(word, func) {word, C_UNARY, {.ucmd = func}}
#define END_CMDS {"XXXX", C_END_OF_LIST, {.ignore = '\0'}}

/* Commands have to follow one of these signatures in order to fit into the
 * command parser - the macro to use is specified in the comment above each
 * typedef.
 */

/* NCMD - nullary command - takes no arguments besides the user data */
typedef enum error (*nullary_cmd_ptr) (void *usr);
/* UCMD - unary command - takes one string argument and user data */
typedef enum error (*unary_cmd_ptr) (void *usr, const char *arg);

/* Type of command, used for the tagged union in struct cmd.  You shouldn't need
 * to use this outside of the macros above in an ideal world.
 */
enum cmd_type {
	C_NULLARY,		/* Command accepts no arguments */
	C_UNARY,		/* Command accepts one argument */
	C_END_OF_LIST		/* Sentinel for end of command list */
};

/* Command structure - you shouldn't ever need to use this directly.  Use the
 * macros above where possible.
 */
struct cmd {
	const char	word [WORD_LEN];	/* Command word */
	enum cmd_type	function_type;	/* Tag for function union */
	union {
		nullary_cmd_ptr	ncmd;	/* No-argument command */
		unary_cmd_ptr	ucmd;	/* One-argument command */
		char		ignore;	/* Use with C_END_OF_LIST */
	}		function;	/* Function pointer to actual command */
};

enum error	check_commands(void *usr, const struct cmd *cmds);

#endif				/* !CUPPA_CMD_H */
