#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>


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

void
convert_to_argument_list (char **args, char *cmd)
{
    int i;
    int count;
    int size;

    size = strlen(cmd); // FIXME: use strnlen()?
    args[0] = cmd;
    for (count = i = 0; i < size; i++) {
        if (cmd[i] == ' ' || cmd[i] == '\t') {
            cmd[i] = '\0';
            args[++count] = cmd + i + 1;
	    if (count > ARG_MAX) {
		fprintf(stderr, "warning: the command exceeds the maximum number of\n"
			        "         arguments (10000) allowed by run4; truncating\n");
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
