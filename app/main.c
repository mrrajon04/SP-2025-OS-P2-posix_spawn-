#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <spawn.h>
#include "../src/lab.h"

extern char **environ;

static void explain_waitpid(int status) {
    if (WIFEXITED(status)) {
        fprintf(stderr, "Child exited with status %d\n", WEXITSTATUS(status));
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr, "Child exited via signal %d\n", WTERMSIG(status));
    }

    if (WIFSTOPPED(status)) {
        fprintf(stderr, "Child stopped by %d\n", WSTOPSIG(status));
    }

    if (WIFCONTINUED(status)) {
        fprintf(stderr, "Child was resumed by delivery of SIGCONT\n");
    }
}

void setup_signal_handlers() {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);
    struct shell sh;
    sh_init(&sh);
    
    // Setup signal handlers to ignore specific signals as required task 8
    setup_signal_handlers();
    
    char *line = (char *)NULL;
    while ((line = readline(sh.prompt))) {
        // do nothing on blank lines, don't save history or attempt to exec
        line = trim_white(line);
        if (!*line) {
            free(line);
            continue;
        }
        add_history(line);
        // check to see if we are launching a built-in command
        char **cmd = cmd_parse(line);
        if (!do_builtin(&sh, cmd)) {
            posix_spawnattr_t attr;
            posix_spawnattr_init(&attr);
            posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
            
            pid_t pid;
            int status;
            if (posix_spawn(&pid, cmd[0], NULL, &attr, cmd, environ) == 0) {
                waitpid(pid, &status, 0);
                explain_waitpid(status);
            } else {
                perror("posix_spawn failed");
            }
            posix_spawnattr_destroy(&attr);
        }
        free(line); // Free the line after processing
        cmd_free(cmd); // Ensure cmd is freed after use
    }
    sh_destroy(&sh);
    exit(EXIT_SUCCESS);
}
