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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "parser.h"

/* Forward declarations */
void rssb_macro_destroy(rssb_macro_t *macro);

/* Implementation */
void
rssb_value_destroy(struct rssb_value *value)
{
  if (value->symbolic)
    free(value->symbol);

  free(value);
}

struct rssb_value *
rssb_value_const(word_t word)
{
  struct rssb_value *val;

  TRYCATCH(val = malloc(sizeof(struct rssb_value)), return NULL);

  val->symbolic = FALSE;
  val->value    = word;

  return val;
}

struct rssb_value *
rssb_value_symbolic(const char *string)
{
  struct rssb_value *val;
  char *dup;

  TRYCATCH(val = malloc(sizeof(struct rssb_value)), return NULL);
  TRYCATCH(dup = strdup(string), free(val); return NULL);

  val->symbolic = TRUE;
  val->symbol   = dup;

  return val;
}

void
rssb_stmt_destroy(rssb_stmt_t *stmt)
{
  unsigned int i;

  if (stmt->string != NULL)
    free(stmt->string);

  for (i = 0; i < stmt->value_count; ++i)
    rssb_value_destroy(stmt->value_list[i]);

  if (stmt->value_list != NULL)
    free(stmt->value_list);

  free(stmt);
}

PRIVATE inline void
rssb_stmt_set_line(rssb_stmt_t *stmt, int line)
{
  stmt->line = line;
}

rssb_stmt_t *
rssb_stmt_new(enum rssb_stmt_type type, const char *string)
{
  rssb_stmt_t *new = NULL;

  TRYCATCH(new = calloc(1, sizeof(rssb_stmt_t)), goto fail);

  if (string != NULL)
    TRYCATCH(new->string = strdup(string), goto fail);

  new->type = type;

  return new;

fail:
  if (new != NULL)
      rssb_stmt_destroy(new);

  return NULL;
}

BOOL
rssb_stmt_push_value(rssb_stmt_t *stmt, word_t word)
{
  struct rssb_value *value = NULL;

  TRYCATCH(value = rssb_value_const(word), goto fail);
  TRYCATCH(PTR_LIST_APPEND_CHECK(stmt->value, value) != -1, goto fail);

  return TRUE;

fail:
  if (value != NULL)
    rssb_value_destroy(value);

  return FALSE;
}

BOOL
rssb_stmt_push_symbol(rssb_stmt_t *stmt, const char *symbol)
{
  struct rssb_value *value = NULL;

  TRYCATCH(value = rssb_value_symbolic(symbol), goto fail);
  TRYCATCH(PTR_LIST_APPEND_CHECK(stmt->value, value) != -1, goto fail);

  return TRUE;

fail:
  if (value != NULL)
    rssb_value_destroy(value);

  return FALSE;
}

rssb_stmt_t *
rssb_stmt_label_new(const char *label)
{
  return rssb_stmt_new(RSSB_STMT_TYPE_LABEL, label);
}

rssb_stmt_t *
rssb_stmt_new_with_args(
    enum rssb_stmt_type type,
    const char *label,
    const arg_list_t *al)
{
  rssb_stmt_t *new = NULL;
  word_t word;
  char c;
  unsigned int i;

  TRYCATCH(new = rssb_stmt_new(type, label), goto fail);

  for (i = 1; i < al->al_argc; ++i) {
    if (strlen(al->al_argv[i]) < 1) {
      fprintf(
          stderr,
          "%s: symbolic argument #%d cannot be empty\n",
          __FUNCTION__,
          i);
      goto fail;
    }

    if (sscanf(al->al_argv[i], "%i", &word) == 1) {
      TRYCATCH(rssb_stmt_push_value(new, word), goto fail);
    } else if (sscanf(al->al_argv[i], "'%c'", &c) == 1) {
      TRYCATCH(rssb_stmt_push_value(new, c), goto fail);
    } else {
      TRYCATCH(rssb_stmt_push_symbol(new, al->al_argv[i]), goto fail);
    }
  }

  return new;

fail:
  if (new != NULL)
    rssb_stmt_destroy(new);

  return NULL;
}

rssb_stmt_t *
rssb_stmt_macro_new(
    enum rssb_stmt_type type,
    const char *label,
    const arg_list_t *al)
{
  return rssb_stmt_new_with_args(RSSB_STMT_TYPE_MACRO, label, al);
}

rssb_stmt_t *
rssb_stmt_inst_new(
    enum rssb_stmt_type type,
    const char *label,
    const arg_list_t *al)
{
  return rssb_stmt_new_with_args(RSSB_STMT_TYPE_INST, label, al);
}

BOOL
rssb_macro_set_put_macro(rssb_macro_set_t *set, rssb_macro_t *macro)
{
  TRYCATCH(PTR_LIST_APPEND_CHECK(set->macro, macro) != -1, return FALSE);

  return TRUE;
}

void
rssb_macro_set_destroy(rssb_macro_set_t *set)
{
  unsigned int i;

  for (i = 0; i < set->macro_count; ++i)
    if (set->macro_list[i] != NULL)
      rssb_macro_destroy(set->macro_list[i]);

  if (set->macro_list != NULL)
    free(set->macro_list);

  free(set);
}

rssb_macro_t *
rssb_macro_set_find_macro(const rssb_macro_set_t *set, const char *name)
{
  unsigned int i;

  for (i = 0; i < set->macro_count; ++i)
    if (set->macro_list[i] != NULL)
      if (strcmp(set->macro_list[i]->name, name) == 0)
        return set->macro_list[i];

  return NULL;
}

rssb_macro_set_t *
rssb_macro_set_new(void)
{
  return (rssb_macro_set_t *) calloc(1, sizeof(rssb_macro_set_t));
}

BOOL
rssb_scope_put_macro(rssb_scope_t *scope, rssb_macro_t *macro)
{
  TRYCATCH(rssb_macro_set_put_macro(scope->macro_set, macro), return FALSE);

  return TRUE;
}

BOOL
rssb_scope_put_stmt(rssb_scope_t *scope, rssb_stmt_t *stmt)
{
  TRYCATCH(PTR_LIST_APPEND_CHECK(scope->stmt, stmt) != -1, return FALSE);

  return TRUE;
}

void
rssb_scope_destroy(rssb_scope_t *scope)
{
  unsigned int i;

  if (scope->macro_set != NULL)
    rssb_macro_set_destroy(scope->macro_set);

  for (i = 0; i < scope->stmt_count; ++i)
    if (scope->stmt_list[i] != NULL)
      rssb_stmt_destroy(scope->stmt_list[i]);

  if (scope->stmt_list != NULL)
    free(scope->stmt_list);

  free(scope);
}

rssb_scope_t *
rssb_scope_new(unsigned int args)
{
  rssb_scope_t *new = NULL;

  TRYCATCH(new = calloc(1, sizeof(rssb_scope_t)), goto fail);
  TRYCATCH(new->macro_set = rssb_macro_set_new(), goto fail);

  if (args != 0)
    TRYCATCH(new->act_args = calloc(args, sizeof(word_t)), goto fail);

  return new;

fail:
  if (new != NULL)
    rssb_scope_destroy(new);

  return NULL;
}

void
rssb_macro_destroy(rssb_macro_t *macro)
{
  if (macro->name != NULL)
    free(macro->name);

  if (macro->args != NULL)
    strlist_destroy(macro->args);

  if (macro->scope != NULL)
    rssb_scope_destroy(macro->scope);

  free(macro);
}

rssb_macro_t *
rssb_macro_new(const char *name, unsigned int args)
{
  rssb_macro_t *new;

  TRYCATCH(new = calloc(1, sizeof(rssb_macro_t)), goto fail);

  TRYCATCH(new->name = strdup(name), goto fail);
  TRYCATCH(new->args = strlist_new(), goto fail);
  TRYCATCH(new->scope = rssb_scope_new(args), goto fail);

  new->scope->owner = new;

  return new;

fail:
  if (new != NULL)
    rssb_macro_destroy(new);

  return NULL;
}

PRIVATE rssb_scope_t *
rssb_macro_get_scope(const rssb_macro_t *macro)
{
  return macro->scope;
}

BOOL
rssb_macro_put_argument(rssb_macro_t *macro, const char *name)
{
  (void) strlist_append_string(macro->args, name);

  return TRUE;
}

PRIVATE arg_list_t *
rssb_split_line(const char *line)
{
  return __split_command(line, " \t,", 0);
}

void
rssb_program_destroy(rssb_program_t *prog)
{
  if (prog->scope != NULL)
    rssb_scope_destroy(prog->scope);

  if (prog->options != NULL)
    strlist_destroy(prog->options);

  free(prog);
}

rssb_program_t *
rssb_program_new(void)
{
  rssb_program_t *new = NULL;

  TRYCATCH(new = calloc(1, sizeof(rssb_program_t)), goto fail);

  TRYCATCH(new->scope = rssb_scope_new(0), goto fail);
  TRYCATCH(new->options = strlist_new(), goto fail);
  return new;

fail:
  if (new != NULL)
    rssb_program_destroy(new);

  return NULL;
}

PRIVATE BOOL
rssb_scope_parse_from_fp(
    rssb_program_t *prog,
    rssb_scope_t *scope,
    FILE *fp)
{
  BOOL ok = FALSE;
  BOOL parsing = TRUE;
  char *line = NULL;
  arg_list_t *al = NULL;
  rssb_stmt_t *stmt = NULL;
  rssb_macro_t *macro = NULL;
  rssb_scope_t *macro_scope;
  unsigned int i;

  while (parsing && (line = fread_line(fp)) != NULL) {
    TRYCATCH(al = rssb_split_line(line), goto done);

    if (al->al_argc > 0) {
      stmt = NULL;
      if (strcmp(al->al_argv[0], "rssb") == 0) {
        if (al->al_argc != 2) {
          fprintf(stderr, "syntax error: invalid RSSB syntax\n");
          goto done;
        }

        TRYCATCH(
            stmt = rssb_stmt_new_with_args(RSSB_STMT_TYPE_INST, NULL, al),
            goto done);
      } else if (strcmp(al->al_argv[0], ".origin") == 0) {
        if (al->al_argc != 2) {
          fprintf(stderr, "syntax error: invalid origin directive\n");
          goto done;
        }

        TRYCATCH(
            stmt = rssb_stmt_new_with_args(RSSB_STMT_TYPE_ORIGIN, NULL, al),
            goto done);
      } else if (strcmp(al->al_argv[0], ".option") == 0) {
        if (al->al_argc != 2) {
          fprintf(stderr, "syntax error: invalid origin directive\n");
          goto done;
        }

        strlist_append_string(prog->options, al->al_argv[1]);
      } else if (strcmp(al->al_argv[0], ".macro") == 0) {
        if (al->al_argc < 2) {
          fprintf(stderr, "syntax error: invalid macro definition syntax\n");
          goto done;
        }

        TRYCATCH(
            macro = rssb_macro_new(al->al_argv[1], al->al_argc - 2),
            goto done);

        for (i = 2; i < al->al_argc; ++i)
          TRYCATCH(
              rssb_macro_put_argument(macro, al->al_argv[i]),
              goto done);

        macro_scope = rssb_macro_get_scope(macro);
        macro_scope->parent = scope; /* TODO: add method to do this */

        TRYCATCH(rssb_scope_put_macro(scope, macro), goto done);
        macro = NULL;

        TRYCATCH(rssb_scope_parse_from_fp(prog, macro_scope, fp), goto done);
      } else if (strcmp(al->al_argv[0], ".end") == 0) {
        if (al->al_argc != 1) {
          fprintf(stderr, "syntax error: invalid macro end syntax\n");
          goto done;
        }

        parsing = FALSE;
      } else if (al->al_argc == 1
          && al->al_argv[0][strlen(al->al_argv[0]) - 1] == ':') {
        al->al_argv[0][strlen(al->al_argv[0]) - 1] = '\0';
        TRYCATCH(
            stmt = rssb_stmt_new(
                RSSB_STMT_TYPE_LABEL,
                al->al_argv[0]),
            goto done);
      } else {
        TRYCATCH(
            stmt = rssb_stmt_new_with_args(RSSB_STMT_TYPE_MACRO, NULL, al),
            goto done);
      }

      if (stmt != NULL) {
        TRYCATCH(rssb_scope_put_stmt(scope, stmt), goto done);
        stmt = NULL;
      }

      free_al(al);
      al = NULL;
    }

    free(line);
  }

  ok = TRUE;

done:
  if (line != NULL)
    free(line);

  if (al != NULL)
    free_al(al);

  if (macro != NULL)
    rssb_macro_destroy(macro);

  if (stmt != NULL)
    rssb_stmt_destroy(stmt);

  return ok;
}

BOOL
rssb_program_load_file(rssb_program_t *prog, const char *path)
{
  FILE *fp = NULL;
  BOOL ok = FALSE;

  if ((fp = fopen(path, "r")) == NULL) {
    fprintf(
        stderr,
        "%s: cannot open file `%s': %s\n",
        __FUNCTION__,
        path,
        strerror(errno));
    goto done;
  }

  TRYCATCH(rssb_scope_parse_from_fp(prog, prog->scope, fp), goto done);

  ok = TRUE;

done:
  if (fp != NULL)
    fclose(fp);

  return ok;
}

/***************************** COMPILATION ***********************************/
PRIVATE void
rssb_scope_reset_unresolved(rssb_scope_t *scope)
{
  scope->unresolved = 0;
}

PRIVATE void
rssb_scope_reset(rssb_scope_t *scope)
{
  unsigned int i;

  if (scope->owner != NULL)
    memset(
        scope->act_args,
        0,
        scope->owner->args->strings_count * sizeof (word_t));

  scope->caller = NULL;
  rssb_scope_reset_unresolved(scope);

  for (i = 0; i < scope->stmt_count; ++i)
    if (scope->stmt_list[i] != NULL
        && scope->stmt_list[i]->type != RSSB_STMT_TYPE_ORIGIN) {
      scope->stmt_list[i]->current_value = 0;
      scope->stmt_list[i]->assembled = FALSE;
    }
}

PRIVATE rssb_macro_t *
rssb_scope_find_macro(const rssb_scope_t *scope, const char *name)
{
  unsigned int i;
  rssb_macro_t *macro;

  if ((macro = rssb_macro_set_find_macro(scope->macro_set, name)) != NULL)
    return macro;

  if (scope->parent != NULL)
    return rssb_scope_find_macro(scope, name);

  return FALSE;
}

PRIVATE BOOL
rssb_scope_resolve_sym(
    rssb_scope_t *scope,
    const rssb_stmt_t *stmt,
    const char *name,
    word_t *word)
{
  int i;
  word_t parsed;

  /* Detect some common expresions */
  if (sscanf(name, "%i", &parsed) == 1) {
    *word = parsed;
    return TRUE;
  } else if (*name == '-') {
    if (!rssb_scope_resolve_sym(scope, stmt, name + 1, &parsed))
      return FALSE;
    *word = -parsed;
    return TRUE;
  } else if (*name == '$') {
    if (!rssb_scope_resolve_sym(scope, stmt, name + 1, &parsed))
      return FALSE;

    *word = parsed + stmt->current_value;
    return TRUE;
  }

  if (strcasecmp(name, "A") == 0 || strcasecmp(name, "AC") == 0) {
    *word = RSSB_ADDR_A;
    return TRUE;
  } else if (strcasecmp(name, "IP") == 0 || strcasecmp(name, "PC") == 0) {
    *word = RSSB_ADDR_IP;
    return TRUE;
  } else if (strcasecmp(name, "ZERO") == 0 || strcasecmp(name, "$0") == 0) {
    *word = RSSB_ADDR_ZERO;
    return TRUE;
  } else if (strcasecmp(name, "IN") == 0 || strcasecmp(name, "INPUT") == 0) {
    *word = RSSB_ADDR_IN;
    return TRUE;
  } else if (strcasecmp(name, "OUT") == 0 || strcasecmp(name, "OUTPUT") == 0) {
    *word = RSSB_ADDR_OUT;
    return TRUE;
  }


  /* It may be a label */
  for (i = scope->stmt_count - 1; i >= 0; --i)
    if (scope->stmt_list[i] != NULL)
      if (scope->stmt_list[i]->type == RSSB_STMT_TYPE_LABEL &&
          strcmp(scope->stmt_list[i]->label, name) == 0) {
        if (!scope->stmt_list[i]->assembled)
          ++scope->unresolved;
        *word = scope->stmt_list[i]->current_value;
        return TRUE;
      }

  /* It may be an argument */
  if (scope->owner != NULL)
    if ((i = strlist_find(scope->owner->args, name)) != -1) {
      *word = scope->act_args[i];
      return TRUE;
    }

  /* Not found in current scope: try parent scope */
  if (scope->parent != NULL)
    return rssb_scope_resolve_sym(scope->parent, stmt, name, word);

  /* Top scope: return false */
  fprintf(stderr, "error: failed to resolve symbol `%s'\n", name);
  return FALSE;
}

PRIVATE BOOL
rssb_scope_get_value(
    rssb_scope_t *scope,
    const rssb_stmt_t *stmt,
    unsigned int i,
    word_t *word)
{
  assert(i < stmt->value_count);

  if (stmt->value_list[i]->symbolic)
    return rssb_scope_resolve_sym(
        scope,
        stmt,
        stmt->value_list[i]->symbol,
        word);

  *word = stmt->value_list[i]->value;

  return TRUE;
}

PRIVATE BOOL
rssb_scope_compile(rssb_scope_t *scope, rssb_vm_t *vm)
{
  unsigned int i, j;
  rssb_macro_t *macro;
  word_t value;
  unsigned int unresolved, last_unresolved;

  for (i = 0; i < scope->stmt_count; ++i)
    if (scope->stmt_list[i] != NULL) {
      scope->stmt_list[i]->assembled     = TRUE;
      scope->stmt_list[i]->current_value = rssb_vm_get_ptr(vm);

      switch (scope->stmt_list[i]->type) {
        case RSSB_STMT_TYPE_INST:
          if (!rssb_scope_get_value(
              scope,
              scope->stmt_list[i],
              0,
              &value)) {
            fprintf(
                stderr,
                "error: failed to retrieve RSSB param at %s+%d\n",
                scope->owner ? scope->owner->name : "<main program>",
                i);
            return FALSE;
          }

          TRYCATCH(
              rssb_vm_put_word(
                  vm,
                  value),
              return FALSE);
          break;

        case RSSB_STMT_TYPE_ORIGIN:
          if (scope->stmt_list[i]->value_list[0]->symbolic) {
            fprintf(
                stderr,
                "error: origin address cannot be symbolic at %s+%d\n",
                scope->owner ? scope->owner->name : "<main program>",
                i);
            return FALSE;
          }

          rssb_vm_set_ptr(vm, scope->stmt_list[i]->value_list[0]->value);
          break;

        case RSSB_STMT_TYPE_LABEL:
          break;

        case RSSB_STMT_TYPE_MACRO:
          if ((macro = rssb_scope_find_macro(
              scope,
              scope->stmt_list[i]->name)) != NULL) {
            fprintf(
                stderr,
                "error: macro `%s' not defined at %s+%d\n",
                scope->stmt_list[i]->name,
                scope->owner ? scope->owner->name : "<main program>",
                i);
            return FALSE;
          }

          if (macro->args->strings_count != scope->stmt_list[i]->value_count) {
            fprintf(
                stderr,
                "error: macro `%s' expects %d args, but only %d were passed at %s+%d\n",
                scope->stmt_list[i]->name,
                macro->args->strings_count,
                scope->stmt_list[i]->value_count,
                scope->owner ? scope->owner->name : "<main program>",
                i);
            return FALSE;
          }

          rssb_scope_reset(macro->scope);
          macro->scope->caller = scope->owner;

          for (j = 0; j < scope->stmt_list[i]->value_count; ++j) {
            if (!rssb_scope_get_value(
                scope,
                scope->stmt_list[i],
                j,
                &value)) {
              fprintf(
                  stderr,
                  "error: failed to resolve %s param #%d at %s+%d\n",
                  scope->stmt_list[i]->name,
                  j,
                  scope->owner ? scope->owner->name : "<main program>",
                  i);
              return FALSE;
            }

            /* Update actual arg #j with current value */
            macro->scope->act_args[j] = value;
          }

          /* Do several passes */
          unresolved = 0;
          do {
            last_unresolved = unresolved;

            rssb_vm_set_ptr(vm, scope->stmt_list[i]->current_value);
            rssb_scope_reset_unresolved(macro->scope);

            if (!rssb_scope_compile(macro->scope, vm)) {
              fprintf(
                  stderr,
                  "error: while calling `%s' at %s+%d\n",
                  scope->stmt_list[i]->name,
                  scope->owner ? scope->owner->name : "<main program>",
                  i);
              return FALSE;
            }

            unresolved = macro->scope->unresolved;
          } while (unresolved > 0 && unresolved != last_unresolved);

          break;

        default:
          fprintf(stderr, "error: what?\n");
          exit(EXIT_FAILURE);
      }
    }

  return TRUE;
}

BOOL
rssb_program_compile(rssb_program_t *prog, rssb_vm_t *vm)
{
  word_t last_addr;
  unsigned int unresolved, last_unresolved;
  unsigned int i;

  if (strlist_have_element(prog->options, "dumb"))
    rssb_vm_set_dumb(vm, TRUE);

  last_addr = rssb_vm_get_ptr(vm);

  unresolved = 0;
  do {
    last_unresolved = unresolved;

    rssb_vm_set_ptr(vm, last_addr);
    rssb_scope_reset_unresolved(prog->scope);
    if (!rssb_scope_compile(prog->scope, vm)) {
      fprintf(stderr, "error: main program compilation failed\n");
      return FALSE;
    }
    unresolved = prog->scope->unresolved;
  } while (unresolved > 0 && unresolved != last_unresolved);

  if (unresolved > 0) {
    fprintf(stderr, "error: unresolved forward declares (how is this even possible?)\n");
    return FALSE;
  }

  return TRUE;
}
