/*
 * main.c: entry point for rssb
 * Creation date: Sat Oct  6 20:01:54 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "parser.h"

#define RSSB_MEMORY_SIZE 65536

int
main (int argc, char *argv[], char *envp[])
{
  rssb_vm_t *vm = NULL;
  rssb_program_t *program = NULL;
  unsigned int i;

  if (argc < 2) {
    fprintf(stderr, "%s: not files given\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if ((program = rssb_program_new()) == NULL) {
    fprintf(stderr, "%s: failed to create program\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if ((vm = rssb_vm_new(RSSB_MEMORY_SIZE)) == NULL) {
    fprintf(stderr, "%s: failed to create RSSB VM\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  for (i = 1; i < argc; ++i) {
    if (!rssb_program_load_file(program, argv[i])) {
      fprintf(stderr, "%s: failed to load source file %s\n", argv[0], argv[i]);
      exit(EXIT_FAILURE);
    }
  }

  if (!rssb_program_compile(program, vm)) {
    fprintf(stderr, "%s: compilation failed\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  (void) rssb_vm_run(vm);

  rssb_vm_destroy(vm);
  rssb_program_destroy(program);

  return 0;
}

