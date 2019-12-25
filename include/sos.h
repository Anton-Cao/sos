#ifndef SOS_H
#define SOS_H

#include <regex.h>

#define MAX_LINE_LEN 256

// error_filter.c
struct error_filter {
  char *name;  // name of filter, for debugging purposes only
  regex_t re;  // regex pattern to match error messages
  char *tags;  // extra text to include in search
  struct error_filter *next;  // pointer to next filter in linked list
};
extern struct error_filter *error_filters;
int ErrorFilter_parse(char*);

#endif // SOS_H
