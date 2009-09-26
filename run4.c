/*
 *  Please see the README.textile for info about this tool.
 *
 *  -----------------------------------------------------------------
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                     Version 2, December 2004
 * 
 *  Copyright (C) 2004 Sam Hocevar
 *   14 rue de Plaisance, 75014 Paris, France
 *  Everyone is permitted to copy and distribute verbatim or modified
 *  copies of this license document, and changing it is allowed as long
 *  as the name is changed.
 * 
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *    TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 * 
 *   0. You just DO WHAT THE FUCK YOU WANT TO. 
 * 
 *  -----------------------------------------------------------------
 *  This program is free software. It comes without any warranty, to
 *  the extent permitted by applicable law. You can redistribute it
 *  and/or modify it under the terms of the Do What The Fuck You Want
 *  To Public License, Version 2, as published by Sam Hocevar. See
 *  http://sam.zoy.org/wtfpl/COPYING for more details.
 *  -----------------------------------------------------------------
 *
 *  Enjoy,
 *  jazzyb
 */

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>


#define ARG_MAX 10000 // totally arbitrary

pid_t cpid;

void
usage (void)
{
    fprintf(stderr, "usage: run4 <number> {seconds|minutes|hours} <command>\n"
                    "  * note that if command is more than one word, it should be quoted\n"
                    "  ex: run4 30 minutes 'tshark -i eth0' > sniff.log\n");
}

int
convert_to_seconds (char *number, char *measure)
{
    int secs = (int)strtol(number, (char **)NULL, 10);
    if (secs < 0) {
        fprintf(stderr, "error: number cannot be negative\n");
        usage();
        exit(1);
    }

    if (strcmp(measure, "hours") == 0) {
        secs *= 3600;
    } else if (strcmp(measure, "minutes") == 0) {
        secs *= 60;
    } else if (strcmp(measure, "seconds") != 0) {
        fprintf(stderr, "error: unknown measure of time `%s'\n", measure);
        usage();
        exit(1);
    }

    return secs;
}

inline int isquote (char c) { return (c == '\'' || c == '"'); }

inline int isseparator (char c) { return (isspace(c) || isquote(c) || c == '\0'); }

/*
 * Sets the given char in the string to an '\0' char to end the previous string
 * in the argument list.  Then adds the next non-empty string to the argument
 * list.
 */
inline int
add_arg_to_list (char *cmd, int i, char **args, int *count)
{
    cmd[i] = '\0';
    if (!isseparator(cmd[i+1])) { // the next string isn't empty
	args[++(*count)] = cmd + i + 1;
    }
}


/*
 * Set each of the char*'s in args to point to each of the words in cmd,
 * ignoring empty strings and preserving quoted strings.
 */
void
convert_to_argument_list (char **args, char *cmd)
{
    int i;
    int count;
    int size;
    char within_quotes;

    args[0] = cmd;
    within_quotes = 0;
    size = strlen(cmd);
    for (count = i = 0; i < size; i++) {
	if (isquote(cmd[i])) {
	    if (cmd[i] == within_quotes) {
		within_quotes = 0;
		add_arg_to_list(cmd, i, args, &count);

	    } else if (!within_quotes) {
		within_quotes = cmd[i];
		add_arg_to_list(cmd, i, args, &count);
	    }

	} else if (isspace(cmd[i]) && !within_quotes) {
	    add_arg_to_list(cmd, i, args, &count);

	    if (count > ARG_MAX) {
		fprintf(stderr, "warning: the command exceeds the maximum number of\n"
				"         arguments (%d) allowed by run4; truncating\n",
				ARG_MAX);
		break;
	    }
	}
    }
    args[count+1] = NULL;
}

void
kill_child (void)
{
    int dummy;

    kill(cpid, SIGINT);
    sleep(1);
    kill(cpid, SIGKILL);
    waitpid(cpid, &dummy, WNOHANG);
}

void
sighandler (int sig)
{
    kill_child();
    exit(0);
}

int
main (int argc, char *argv[])
{
    int i;
    int time;
    char *args[ARG_MAX+1];

    if (argc != 4) {
        usage();
        if (argc == 2 && strcmp(argv[1], "-h") == 0) {
            exit(0);
        } else {
            exit(1);
        }
    }

    time = convert_to_seconds(argv[1], argv[2]);
    convert_to_argument_list(args, argv[3]);

    cpid = fork();
    if (cpid == 0) {
        execvp(args[0], args);
        // if execvp() returns, then something failed:
        fprintf(stderr, "error: unable to exec the command\n");
        exit(1);

    } else if (cpid > 0) {
        signal(SIGINT, sighandler);
        signal(SIGTERM, sighandler);
        sleep(time);
        kill_child();

    } else {
        fprintf(stderr, "error: unable to fork\n");
        exit(1);
    }

    return 0;
}
