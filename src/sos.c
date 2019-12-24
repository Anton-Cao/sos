#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <curl/curl.h>

#include "ketopt.h"

#define OPEN_BROWSER_OPT 301  // should be larger than max ASCII code
#define MAX_COMMAND_TOKENS 8
#define MAX_LINE_LEN 1024

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
    int i = 0;
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
    printf("(sos) running [%s] with pid=%d\n", command, pid);
    char c;
    char buf[MAX_LINE_LEN];
    char errmsg[MAX_LINE_LEN];
    char url[MAX_LINE_LEN];
    int i = 0;
    CURL *curl = curl_easy_init();
    while (read(masterpt, &c, sizeof(c))) {
      if (c == '\n') {
        buf[i] = 0;  // null-terminate
        i = 0;  // reset
        printf("%s\n", buf);
        regex_t re;
        regmatch_t rms[2];
        int status;
        regcomp(&re, "Error: (.*)$", REG_EXTENDED | REG_ICASE);
        status = regexec(&re, buf, 2, rms, 0);
        regfree(&re);
        if (status == 0) {
          // match found
          size_t len = rms[1].rm_eo - rms[1].rm_so;
          strncpy(errmsg, buf + rms[1].rm_so, len);
          errmsg[len] = 0;
          char *query = curl_easy_escape(curl, errmsg, len);
          snprintf(url, MAX_LINE_LEN, "https://stackoverflow.com/search?q=%s", query);
          printf("(sos) \033[91m%s\033[0m\n", url);
          curl_free(query);
        }
      } else {
        buf[i++] = c;
      }
    }
    #ifdef DEBUG
    printf("child exited\n");
    #endif
  }

  return 0;
}
