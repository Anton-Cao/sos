#include "sos.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

const char *SOSRC_FILE = "/.sosrc";
const char *NAME = "name: ";
const char *PATTERN = "pattern: ";
const char *TAGS = "tags: ";

struct error_filter *error_filters = NULL;

// Allocate space for a error filter. Add it to the linked list.
// Return a pointer to the new error filter.
struct error_filter* ErrorFilter_new()
{
  struct error_filter *filter = malloc(sizeof(struct error_filter));
  filter->name = NULL;
  filter->next = error_filters;
  error_filters = filter;
  return filter;
}

// Parse a line of the sosrc file and update the error filter.
// Return -1 on error, 0 on success.
int ErrorFilter_add_line(struct error_filter *filter, char *line)
{
  char *start;
  if (start=strstr(line, NAME)) {
    filter->name = malloc(strlen(start) - strlen(NAME) + 1);
    strcpy(filter->name, start + strlen(NAME));
#ifdef DEBUG
    printf("name: %s\n", filter->name);
#endif
  } else if (start=strstr(line, PATTERN)) {
    char *restr = malloc(strlen(start) - strlen(PATTERN) + 1);
    strcpy(restr, start + strlen(PATTERN));
    regcomp(&filter->re, restr, REG_EXTENDED | REG_ICASE);
#ifdef DEBUG
    printf("pattern: %s\n", restr);
#endif
    if (filter->re.re_nsub != 1) {
      fprintf(stderr,
              "pattern must have exactly one parenthesized subexpression, found \"%s\"\n", restr);
      free(restr);
      return -1;
    }
    free(restr);
  } else if (start=strstr(line, TAGS)) {
    filter->tags = malloc(strlen(start) - strlen(TAGS) + 1);
    strcpy(filter->tags, start + strlen(TAGS));
#ifdef DEBUG
    printf("tags: %s\n", filter->tags);
#endif
  }
  return 0;
}

// Parse sosrc file to populate error filters linked list.
// Return -1 on error, 0 on success.
int ErrorFilter_parse(char *sosrc_path)
{
  // open sosrc file
  bool allocated = false;
  if (!sosrc_path) {
    if (!getenv("HOME")) {
      fprintf(stderr, "$HOME not set, cannot find ~/.sosrc\n");
      return -1;
    }
    sosrc_path = malloc(strlen(getenv("HOME")) + strlen(SOSRC_FILE) + 1);
    allocated = true;
    strcat(strcpy(sosrc_path, getenv("HOME")), SOSRC_FILE);
  }
  int sosrc = open(sosrc_path, O_RDONLY);
  if (allocated)
    free(sosrc_path);
  if (sosrc == -1) {
    fprintf(stderr, "unable to open %s, check that it exists?\n", sosrc_path);
    return -1;
  }

  // parse each line of sosrc file
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
      if (ErrorFilter_add_line(cur_filter, buf) == -1)
        return -1;
    } else {
      if (i >= MAX_LINE_LEN) {
        fprintf(stderr, "maximum line length exceeded on line %d of %s\n", linenum, SOSRC_FILE);
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
