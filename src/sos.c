#include "sos.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <curl/curl.h>

#include "ketopt.h"

#define MAX_COMMAND_TOKENS 8

const char *prompt = "\033[34m(sos)\033[0m";

void child(struct cmd_opts *cmd_opts, int minionpt)
{
  char *token = strtok(cmd_opts->command, " ");
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
  printf("command: %s\n", cmd_opts->command);
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
  execvp(argv[0], (char * const *) argv);
}

void parent(struct cmd_opts *cmd_opts, int masterpt)
{
  int i = 0, status, len;
  char c,
    buf[MAX_LINE_LEN],
    err[MAX_LINE_LEN],
    *query,
    url[MAX_LINE_LEN];
  struct error_filter *filter;
  regmatch_t rms[2];
  CURL *curl = curl_easy_init();

  while (read(masterpt, &c, sizeof(c))) {
    if (c == '\n' || c == '\r') {
      buf[i] = 0;

      if (i > 0) {
        printf("%s\n", buf);

        // traverse linked list
        filter = error_filters;
        while (filter) {
          status = regexec(&filter->re, buf, 2, rms, 0);
          if (status == 0) {  // match found
            len = rms[1].rm_eo - rms[1].rm_so;
            strncpy(err, buf + rms[1].rm_so, len);
            err[len] = 0;
            strcat(err, " ");
            strcat(err, filter->tags);
            query = curl_easy_escape(curl, err, 0);
            snprintf(url, MAX_LINE_LEN, "https://stackoverflow.com/search?q=%s", query);
            curl_free(query);
            printf("%s \033[31m%s\033[0m (%s)\n", prompt, url, err);
            if (cmd_opts->open_browser) {
              if (fork() == 0) {  // spawn new process to open in browser
                char *argv[] = {"open", url, NULL};
                execvp(argv[0], argv);
              }
            }
            break;
          }
          filter = filter->next;
        }
      }

      i = 0;
    } else {
      buf[i++] = c;
    }
  }
#ifdef DEBUG
  printf("child exited\n");
#endif
}

int main(int argc, char *argv[])
{
  struct cmd_opts cmd_opts;
  if (CmdOpts_parse(&cmd_opts, argc, argv) == -1 ||
      cmd_opts.command == NULL)
    return -1;

  if (ErrorFilter_parse(cmd_opts.sosrc_path) == -1)
    return -1;

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
    child(&cmd_opts, minionpt);
  } else {
    // parent process
    close(minionpt);
    printf("%s running [%s] with pid=%d\n", prompt, cmd_opts.command, pid);
    parent(&cmd_opts, masterpt);
  }

  return 0;
}
