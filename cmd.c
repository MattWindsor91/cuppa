/*******************************************************************************
 * cmd.c - command parser
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

#include <ctype.h>
#include <stdbool.h>		/* bool */
#include <stdio.h>		/* getline */
#include <stdlib.h>
#include <string.h>

#include "constants.h"		/* WORD_LEN */
#include "cmd.h"		/* struct cmd, enum cmd_type */
#include "errors.h"		/* error */
#include "io.h"			/* response */
#include "messages.h"		/* Messages (usually errors) */

static enum error
exec_cmd(void *usr,
	 const struct cmd *cmds,
	 const char *word,
	 const char *arg);
static enum error
exec_cmd_struct(void *usr,
		const struct cmd *cmd,
		const char *arg);

/*
 * Checks to see if there is a commands waiting on stdio and, if there is,
 * sends it to the command handler.
 * 
 * 'usr' is a pointer to any user data that should be passed to executed
 * commands; 'cmds' is a pointer to an END_CMDS-terminated array of command
 * definitions (see cmd.h for details).
 */
enum error
check_commands(void *usr, const struct cmd *cmds)
{
	enum error	err = E_OK;

	if (input_waiting())
		err = handle_cmd(usr, cmds);

	return err;
}

/* Processes the command currently waiting at standard input. */
enum error
handle_cmd(void *usr, const struct cmd *cmds)
{
	size_t		length;
	enum error	err = E_OK;
	char           *buffer = NULL;
	char           *argument = NULL;
	size_t		num_bytes = 0;

	length = getline(&buffer, &num_bytes, stdin);
	dbug("got command: %s", buffer);

	/* Remember to count newline */
	if (length < WORD_LEN)
		err = error(E_BAD_COMMAND, MSG_CMD_NOWORD);
	if (err == E_OK) {
		/* Find start of argument(s) */
		size_t		i;
		size_t		j;

		for (i = WORD_LEN - 1; i < length && argument == NULL; i++) {
			if (!isspace((int)buffer[i])) {
				/* Assume this is where the arg is */
				argument = buffer + i;
				break;
			}
		}

		/*
		 * Strip any whitespace out of the argument (by setting it to
		 * the null character, thus null-terminating the argument)
		 */
		for (j = length - 1; isspace((int)buffer[j]); i--)
			buffer[j] = '\0';

		err = exec_cmd(usr, cmds, buffer, argument);
		if (err == E_OK)
			response(R_OKAY, "%s", buffer);
	}
	dbug("command processed");
	free(buffer);

	return err;
}

static enum error
exec_cmd(void *usr, const struct cmd *cmds, const char *word, const char *arg)
{
	bool		gotcmd;
	const struct cmd *cmd;
	enum error	err = E_OK;

	for (cmd = cmds, gotcmd = false;
	     cmd->function_type != C_END_OF_LIST && !gotcmd;
	     cmd++) {
		if (cmd->word == ANY ||
		    strncmp(cmd->word, word, WORD_LEN - 1) == 0) {
			gotcmd = true;
			err = exec_cmd_struct(usr, cmd, arg);
		}
	}

	if (!gotcmd)
		err = error(E_BAD_COMMAND, "%s", MSG_CMD_NOSUCH);

	return err;
}

static enum error
exec_cmd_struct(void *usr, const struct cmd *cmd, const char *arg)
{
	enum error	err = E_OK;

	switch (cmd->function_type) {
	case C_NULLARY:	/* No arguments */
		if (arg == NULL)
			err = cmd->function.ncmd(usr);
		else
			err = error(E_BAD_COMMAND, "%s", MSG_CMD_ARGN);
		break;
	case C_UNARY:		/* One argument */
		if (arg == NULL)
			err = error(E_BAD_COMMAND, "%s", MSG_CMD_ARGU);
		else
			err = cmd->function.ucmd(usr, arg);
		break;
	case C_REJECT:		/* Throw a wobbly */
		err = error(E_COMMAND_REJECTED, "%s", cmd->function.reason);
		break;
	case C_END_OF_LIST:
		err = error(E_INTERNAL_ERROR, "%s", MSG_CMD_HITEND);
		break;
	}

	return err;
}
