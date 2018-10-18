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

#ifndef _MAIN_INCLUDE_H
#define _MAIN_INCLUDE_H

#include <config.h> /* General compile-time configuration parameters */
#include <stdint.h>
#include <util.h>

#define PRIVATE static

#define TRYCATCH(expr, action)  \
  if (!(expr)) {                \
    fprintf(                    \
      stderr,                   \
      "%s:%d: exception in expression " STRINGIFY(expr) "\n", \
      __FILE__,                 \
      __LINE__);                \
    action;                     \
  }

typedef uint32_t word_t;

enum rssb_bool {
  FALSE,
  TRUE
};

typedef enum rssb_bool BOOL;

enum rssb_addr {
  RSSB_ADDR_IP,
  RSSB_ADDR_A,
  RSSB_ADDR_ZERO,
  RSSB_ADDR_IN,
  RSSB_ADDR_OUT,
  RSSB_ADDR_MIN
};

typedef struct rssb_vm {
  BOOL dumb_mode;
  word_t *mem;
  word_t footprint;
  unsigned int mem_neg_mask;
  unsigned int mem_mask;
  unsigned int mem_size;
  unsigned int mem_ptr;

  void *private;
  BOOL (*input) (void *private, word_t *ch);
  BOOL (*output) (void *private, word_t ch);
} rssb_vm_t;

rssb_vm_t *rssb_vm_new(unsigned int size);
BOOL   rssb_vm_put_word(rssb_vm_t *vm, word_t word);
word_t rssb_vm_get_ptr(const rssb_vm_t *vm);
void   rssb_vm_disas(const rssb_vm_t *vm);
void   rssb_vm_set_ptr(rssb_vm_t *vm, word_t ptr);
void   rssb_vm_set_dumb(rssb_vm_t *vm, BOOL dumb);
BOOL   rssb_vm_run(rssb_vm_t *vm);
void   rssb_vm_destroy(rssb_vm_t *vm);

#endif /* _MAIN_INCLUDE_H */
