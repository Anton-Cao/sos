#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "ketopt.h"

#define OPEN_BROWSER_OPT 301
#define MAX_COMMAND_TOKENS 8

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
  bool openbrowser = false;
  char *command;
  while ((c = ketopt(&opt, argc, argv, 1, "", longopts)) >= 0) {
    switch(c)
      {
      case OPEN_BROWSER_OPT:
        openbrowser = true;
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

  int i;

  // O_NOCTTY because we don't want parent process to be controlled by the pseudo-terminal
  // https://linux.die.net/man/3/posix_openpt
  // https://stackoverflow.com/questions/4057985/disabling-stdout-buffering-of-a-forked-process
  // https://pubs.opengroup.org/onlinepubs/009695399/functions/posix_openpt.html
  int masterpt, minionpt;
  char *miniondevice;
  masterpt = posix_openpt(O_RDWR | O_NOCTTY);
  if (masterpt == -1
      || grantpt(masterpt) == -1
      || unlockpt(masterpt) == -1
      || (miniondevice = ptsname(masterpt)) == NULL) {
    fprintf(stderr, "error creating pseudo-terminal\n");
    return -1;
  }
  #ifdef DEBUG
  printf("minion device is %s\n", miniondevice);
  #endif
  minionpt = open(miniondevice, O_RDWR | O_NOCTTY);

  int pid = fork();
  if (pid == 0) {
    // child process
    close(masterpt);
    char *token = strtok(command, " ");
    char *argv[MAX_COMMAND_TOKENS];
    i = 0;
    while (token != NULL) {
      if (i >= MAX_COMMAND_TOKENS) {
        fprintf(stderr, "maximum token limit exceeded\n");
        exit(-1);
      }
      argv[i++] = token;
      token = strtok(NULL, " ");
    }
    argv[i] = NULL;
    #ifdef DEBUG
    printf("command: %s\n", command);
    for (int j = 0; j < i; j++)
      printf("arg: %s\n", argv[j]);
    #endif
    close(STDIN_FILENO);
    dup(minionpt);
    close(STDOUT_FILENO);
    dup(minionpt);
    close(STDERR_FILENO);
    dup(minionpt);
    #ifdef DEBUG
    printf("entering the warp zone...\n");  // this will be written to command_stdout
    #endif
    execvp(command, (char * const *) argv);
  } else {
    // parent process
    close(minionpt);
    printf("running [%s] with pid=%d\n", command, pid);
    char tmp;
    while (read(masterpt, &tmp, sizeof(tmp))) {
      printf("%c", tmp);
    }
    // waitpid(pid, NULL, 0);
    #ifdef DEBUG
    printf("child exited\n");
    #endif
  }

  return 0;
}
