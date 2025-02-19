#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>

#define ARG_MAX sysconf(_SC_ARG_MAX)

char *get_prompt(const char *env) {
    char *prompt = getenv(env);
    if (!prompt) {
        prompt = "shell>";
    }
    return strdup(prompt);
}

int change_dir(char **dir) {
    if (dir[1] == NULL) {
        char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : NULL;
        }
        if (!home) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
        return chdir(home);
    }
    return chdir(dir[1]);
}

char **cmd_parse(char const *line) {
    if (!line || *line == '\0') return NULL;

    char **args = malloc(ARG_MAX * sizeof(char *));
    if (!args) return NULL;

    int i = 0;
    char *token;
    char *line_copy = strdup(line);
    char *saveptr;
    
    token = strtok_r(line_copy, " ", &saveptr);
    while (token && i < ARG_MAX - 1) {
        args[i++] = strdup(token);
        token = strtok_r(NULL, " ", &saveptr);
    }
    args[i] = NULL;
    free(line_copy);
    return args;
}

void cmd_free(char **cmd) {
    if (!cmd) return;
    for (int i = 0; cmd[i]; i++) {
        free(cmd[i]);
    }
    free(cmd);
}

char *trim_white(char *line) {
    if (!line) return NULL;
    
    char *end;
    while (isspace((unsigned char) *line)) line++;
    
    if (*line == 0) return line;
    
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char) *end)) end--;
    
    *(end + 1) = '\0';
    return line;
}

bool do_builtin(struct shell *sh, char **argv) {
    if (!argv || !argv[0]) return false;

    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        return change_dir(argv) == 0;
    }
    return false;
}

void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Could not put the shell in its own process group");
            exit(1);
        }

        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }
    sh->prompt = get_prompt("MY_PROMPT");
}

void sh_destroy(struct shell *sh) {
    free(sh->prompt);
}

void parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
            exit(0);
        }
    }
}
