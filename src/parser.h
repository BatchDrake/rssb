/*

  Copyright (C) 2018 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#ifndef _RSSB_PARSER_H
#define _RSSB_PARSER_H

#include <util.h>

#include "rssb.h"

enum rssb_stmt_type {
  RSSB_STMT_TYPE_INST,
  RSSB_STMT_TYPE_LABEL,
  RSSB_STMT_TYPE_MACRO,
  RSSB_STMT_TYPE_ORIGIN,
};

struct rssb_value {
  BOOL symbolic;

  union {
    word_t value;
    char *symbol;
  };
};

typedef struct rssb_stmt {
  enum rssb_stmt_type type;
  int line;

  BOOL   assembled;
  word_t current_value;

  union {
    char *label;
    char *name;
    char *string;
  };

  PTR_LIST(struct rssb_value, value);
} rssb_stmt_t;

struct rssb_macro_set;
struct rssb_macro;

typedef struct rssb_scope {
  struct rssb_scope *parent;
  struct rssb_macro *owner;
  struct rssb_macro_set *macro_set;

  /* Dynamic members */
  unsigned int unresolved; /* Unresolved labels */
  struct rssb_macro *caller;
  word_t *act_args;
  PTR_LIST(rssb_stmt_t, stmt);
} rssb_scope_t;

typedef struct rssb_macro {
  char *name;
  struct strlist *args;
  rssb_scope_t *scope;
} rssb_macro_t;

typedef struct rssb_macro_set {
  PTR_LIST(rssb_macro_t, macro);
} rssb_macro_set_t;

typedef struct rssb_program {
  word_t origin;
  rssb_scope_t *scope;
  struct strlist *options;
} rssb_program_t;

void rssb_program_destroy(rssb_program_t *prog);
rssb_program_t *rssb_program_new(void);

BOOL rssb_program_compile(rssb_program_t *prog, rssb_vm_t *vm);
BOOL rssb_program_load_file(rssb_program_t *prog, const char *path);

#endif /* _RSSB_PARSER_H */
