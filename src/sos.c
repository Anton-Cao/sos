#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ketopt.h"

#define OPEN_BROWSER_OPT 301

void print_usage()
{
  fprintf(stderr, "usage: sos [--open-browser] [command]\n");
}

int main(int argc, char *argv[])
{
  static ko_longopt_t longopts[] =
    {
     { "open-browser", ko_no_argument, OPEN_BROWSER_OPT }
    };
  ketopt_t opt = KETOPT_INIT;
  int c;
  bool open_browser = false;
  char *command;
  while ((c = ketopt(&opt, argc, argv, 1, "", longopts)) >= 0) {
    switch(c)
      {
      case OPEN_BROWSER_OPT:
        open_browser = true;
        break;
      case '?':
        fprintf(stderr, "unknown opt: -%c\n", opt.opt);
        print_usage();
        return -1;
      case ':':
        fprintf(stderr, "missing arg: -%c\n", opt.opt);
        print_usage();
        return -1;
      }
  }
  if (opt.ind != argc - 1) {
    fprintf(stderr, "found %d command(s), expected 1\n", argc - opt.ind);
    print_usage();
    return -1;
  }

  command = argv[opt.ind];

  int command_stdin[2];
  int command_stdout[2];
  int command_stderr[2];
  pipe(command_stdin);
  pipe(command_stdout);
  pipe(command_stderr);
  int pid = fork();
  if (pid == 0) {
    // child process
    char *token = strtok(command, " ");
    char **argv = &token;
    while (token != NULL)
      token = strtok(NULL, " ");
    close(STDIN_FILENO);
    dup(command_stdin[0]);
    close(STDOUT_FILENO);
    dup(command_stdout[1]);
    close(STDERR_FILENO);
    dup(command_stderr[1]);
    execvp(command, (char * const *) argv);
  } else {
    // parent process
    printf("running %s (pid=%d)\n", command, pid);
    waitpid(pid, NULL, 0);
    printf("child exited\n");
  }

  return 0;
}
