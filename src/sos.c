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
#define DEFAULT_SOSRC_OPT 302
#define HELP_OPT 303

#define MAX_COMMAND_TOKENS 8
#define MAX_LINE_LEN 256

static const char *sosrc_file = "/.sosrc";

struct error_filter {
  char *name;  // name of filter, for debugging purposes only
  regex_t re;  // regex pattern to match error messages
  char *tags;  // extra text to include in search
  struct error_filter *next;  // pointer to next filter in linked list
};

struct error_filter *error_filters = NULL;

void print_usage()
{
  printf("Usage: sos [options] command\n");
  printf("Options:\n");
  printf("  --open-browser (-o): open links automatically in default browser\n");
  printf("  --default-sosrc (-r): print default ~/.sosrc file\n");
  printf("  --help (-h): display this help text\n");
}

void print_default_sosrc()
{
  printf("- name: Python errors\n");
  printf("  pattern: ^(.*Error: .*)$\n");
  printf("  tags: python\n");
}

struct error_filter* ErrorFilter_new()
{
  struct error_filter *filter = malloc(sizeof(struct error_filter));
  filter->name = NULL;
  filter->next = error_filters;
  error_filters = filter;
  return filter;
}

void ErrorFilter_add_line(struct error_filter *filter, char *line)
{
  char const *name = "name: ";
  char const *pattern = "pattern: ";
  char const *tags = "tags: ";
  char *start;
  if (start=strstr(line, name)) {
    filter->name = malloc(strlen(start) - strlen(name) + 1);
    strcpy(filter->name, start + strlen(name));
    #ifdef DEBUG
    printf("name: %s\n", filter->name);
    #endif
  } else if (start=strstr(line, pattern)) {
    char *restr = malloc(strlen(start) - strlen(pattern) + 1);
    strcpy(restr, start + strlen(pattern));
    regcomp(&filter->re, restr, REG_EXTENDED | REG_ICASE);
#ifdef DEBUG
    printf("pattern: %s\n", restr);
#endif
    free(restr);
  } else if (start=strstr(line, tags)) {
    filter->tags = malloc(strlen(start) - strlen(tags) + 1);
    strcpy(filter->tags, start + strlen(tags));
#ifdef DEBUG
    printf("tags: %s\n", filter->tags);
#endif
  }
}

int init_sos(char *sosrc_path)
{
  if (!sosrc_path) {
    if (!getenv("HOME")) {
      fprintf(stderr, "$HOME not set, cannot find ~/.sosrc\n");
      return -1;
    }
    sosrc_path = malloc(strlen(getenv("HOME")) + strlen(sosrc_file) + 1);
    strcat(strcpy(sosrc_path, getenv("HOME")), sosrc_file);
  }
  int sosrc;
  if ((sosrc=open(sosrc_path, O_RDONLY)) == -1) {
    fprintf(stderr, "unable to open %s, check that it exists?\n", sosrc_path);
    return -1;
  }
  char buf[MAX_LINE_LEN];
  char c;
  int i = 0;
  int linenum = 1;
  struct error_filter *cur_filter;
  while (read(sosrc, &c, sizeof(c))) {
    if (c == '\n') {
      buf[i] = 0;
      i = 0;
      linenum++;
      if (buf[0] == '-')
        cur_filter = ErrorFilter_new();
      ErrorFilter_add_line(cur_filter, buf);
    } else {
      if (i >= MAX_LINE_LEN) {
        fprintf(stderr, "maximum line length exceeded on line %d of %s\n", linenum, sosrc_file);
        return -1;
      }
      buf[i++] = c;
    }
  }
  if (!error_filters) {
    fprintf(stderr, "no error filters found in %s\n", sosrc_path);
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  static ko_longopt_t longopts[] =
    {
     { "open-browser", ko_no_argument, OPEN_BROWSER_OPT },
     { "default-sosrc", ko_no_argument, DEFAULT_SOSRC_OPT },
     { "help", ko_no_argument, HELP_OPT },
    };
  ketopt_t opt = KETOPT_INIT;
  int c;
  bool open_browser = false;
  char *command;
  while ((c = ketopt(&opt, argc, argv, 1, "orh", longopts)) >= 0) {
    switch(c)
      {
      case OPEN_BROWSER_OPT:
      case 'o':
        open_browser = true;
        break;
      case DEFAULT_SOSRC_OPT:
      case 'r':
        print_default_sosrc();
        return 0;
      case HELP_OPT:
      case 'h':
        print_usage();
        return 0;
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

  if (init_sos(NULL) == -1)
    return -1;

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
        struct error_filter *filter = error_filters;
        regmatch_t rms[2];
        int status;
        while (filter) {
          status = regexec(&filter->re, buf, 2, rms, 0);
          if (status == 0) {
            // match found
            size_t len = rms[1].rm_eo - rms[1].rm_so;
            strncpy(errmsg, buf + rms[1].rm_so, len);
            errmsg[len] = 0;
            char *query = curl_easy_escape(curl, errmsg, len);
            snprintf(url, MAX_LINE_LEN, "https://stackoverflow.com/search?q=%s", query);
            printf("(sos) \033[91m%s\033[0m\n", url);
            curl_free(query);
            if (open_browser) {
              printf("(sos) opening in browser...\n");
              if (fork() == 0) {
                // spawn new process to open in browser
                char *argv[] = {"open", url, NULL};
                execvp(argv[0], argv);
              }
            }
            break;
          }
          filter = filter->next;
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
