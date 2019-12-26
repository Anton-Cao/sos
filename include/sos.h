#ifndef SOS_H
#define SOS_H

#include <regex.h>
#include <stdbool.h>

#define MAX_LINE_LEN 256

// cmd.c
struct cmd_opts {
  char *command;  // sub-command to run
  char *sosrc_path;  // custom sosrc
  bool open_browser;  // open links automatically
};
// parse command line arguments
int CmdOpts_parse(struct cmd_opts *cmd_opts, int argc, char* argv[]);

// error_filter.c
struct error_filter {
  char *name;  // name of filter, for debugging purposes only
  regex_t re;  // regex pattern to match error messages
  char *tags;  // extra text to include in search
  struct error_filter *next;  // pointer to next filter in linked list
};
extern struct error_filter *error_filters;
int ErrorFilter_parse(char *sosrc_path);  // read in error filters from sosrc file

#endif // SOS_H
