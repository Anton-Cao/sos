#include "sos.h"

#include <stdio.h>
#include <stdlib.h>

#include "ketopt.h"

// should be larger than max ASCII code
#define OPEN_BROWSER_OPT 301
#define SOSRC_FILE_OPT 302
#define HELP_OPT 303

const ko_longopt_t longopts[] =
  {
   { "open-browser", ko_no_argument, OPEN_BROWSER_OPT },
   { "sosrc-file", ko_required_argument, SOSRC_FILE_OPT },
   { "help", ko_no_argument, HELP_OPT },
  };

void print_usage()
{
  printf("Usage: sos [options] <command>\n");
  printf("Options:\n");
  printf("  --open-browser (-o): open links automatically in default browser\n");
  printf("  --sosrc-file (-f) <sosrc>: use <sosrc> instead of ~/.sosrc\n");
  printf("  --help (-h): display this help text\n");
}

int CmdOpts_parse(struct cmd_opts *cmd_opts, int argc, char *argv[])
{
  cmd_opts->command = NULL;
  cmd_opts->open_browser = false;
  cmd_opts->sosrc_path = NULL;

  ketopt_t opt = KETOPT_INIT;
  int c;

  while ((c = ketopt(&opt, argc, argv, 1, "of:h", longopts)) >= 0) {
    switch(c)
      {
      case OPEN_BROWSER_OPT:
      case 'o':
        cmd_opts->open_browser = true;
        break;
      case SOSRC_FILE_OPT:
      case 'f':
        cmd_opts->sosrc_path = malloc((strlen(opt.arg) + 1) * sizeof(char));
        strcpy(cmd_opts->sosrc_path, opt.arg);
        break;
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
    fprintf(stderr, "found %d commands, expected 1\n", argc - opt.ind);
    print_usage();
    return -1;
  }
  cmd_opts->command = argv[opt.ind];
  return 0;
}
