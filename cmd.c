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
	 const char *arg,
	 FILE *prop);
static enum error
exec_cmd_struct(void *usr,
		const struct cmd *cmd,
		const char *word,
		const char *arg,
		FILE *prop);

/*
 * Checks to see if there is a command waiting on stdin and, if there is,
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
		err = handle_cmd(usr, cmds, stdin, NULL);

	return err;
}
/* Processes the command currently waiting on the given stream.
 * If the command is set to be handled by PROPAGATE, it will be sent through
 * prop; it is an error if prop is NULL and PROPAGATE is reached.
 */
enum error
handle_cmd(void *usr, const struct cmd *cmds, FILE *in, FILE *prop)
{
	ssize_t		length;
	enum error	err = E_OK;
	char           *buffer = NULL;
	char           *word = NULL;
	char           *arg = NULL;
	char	       *end = NULL;
	size_t		num_bytes = 0;

	length = getline(&buffer, &num_bytes, in);
	dbug("got command: %s", buffer);

	/* Silently fail if the command is actually end of file */
	if (length == -1) {
		dbug("end of file");
		err = E_EOF;
	}
	if (err == E_OK) {
		end = (buffer + length); 
	
		/* Drop leading whitespace and find word */
		for (word = buffer; word < end && isspace((int)*word); word++)
			*word = '\0';
		if (word >= end)
			err = error(E_BAD_COMMAND, MSG_CMD_NOWORD);
	}
	if (err == E_OK) {
		char		*ws;

		/* Skip command and following space to find argument, if any */
		for (arg = word; arg < end && !isspace((int)*arg); arg++)
			;
		for (; arg < end && isspace((int)*arg); arg++)
			*arg = '\0';
		if (arg >= end)
			arg = NULL;
		
		/*
		 * Strip any whitespace out of the argument (by setting it to
		 * the null character, thus null-terminating the argument)
		 */
		for (ws = end - 1; isspace((int)*ws); ws--)
			*ws = '\0';

		err = exec_cmd(usr, cmds, word, arg, prop);
		
		if (err == E_OK) {
			if (arg == NULL)
				response(R_OKAY, "%s", word);
			else
				response(R_OKAY, "%s %s", word, arg);
		} else if (err == E_COMMAND_IGNORED)
			err = E_OK;
	}
	dbug("command processed");
	free(buffer);

	return err;
}

static enum error
exec_cmd(void *usr,
	 const struct cmd *cmds,
	 const char *word,
	 const char *arg,
	 FILE *prop)
{
	bool		gotcmd;
	const struct cmd *cmd;
	enum error	err = E_OK;

	for (cmd = cmds, gotcmd = false;
	     cmd->function_type != C_END_OF_LIST && !gotcmd;
	     cmd++) {
		if (cmd->word == ANY ||
		    strcmp(cmd->word, word) == 0) {
			gotcmd = true;
			err = exec_cmd_struct(usr, cmd, word, arg, prop);
		}
	}

	if (!gotcmd)
		err = error(E_BAD_COMMAND, "%s", MSG_CMD_NOSUCH);

	return err;
}

static enum error
exec_cmd_struct(void *usr,
		const struct cmd *cmd,
		const char *word,
		const char *arg,
		FILE *prop)
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
	case C_IGNORE:
		err = E_COMMAND_IGNORED;
		break;
	case C_PROPAGATE:
		if (prop == NULL)
			err = error(E_INTERNAL_ERROR, "%s", MSG_CMD_NOPROP);
		else {
			if (arg == NULL)
				fprintf(prop, "%s\n", word);
			else
				fprintf(prop, "%s %s\n", word, arg);
			fflush(prop);
		}
		err = E_COMMAND_IGNORED;
		break;
	case C_END_OF_LIST:
		err = error(E_INTERNAL_ERROR, "%s", MSG_CMD_HITEND);
		break;
	}

	return err;
}
