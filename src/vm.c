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

#include "rssb.h"

BOOL
rssb_vm_put_word(rssb_vm_t *vm, word_t word)
{
  if (vm->mem_ptr >= vm->mem_size) {
    fprintf(stderr, "rssb_vm: out of memory putting a word\n");
    return FALSE;
  }

  if (vm->mem_ptr > vm->footprint)
    vm->footprint = vm->mem_ptr;

  vm->mem[vm->mem_ptr++] = word;
  return TRUE;
}

void
rssb_vm_disas(const rssb_vm_t *vm)
{
  unsigned int i;

  for (i = 0; i <= vm->footprint; ++i)
    printf("0x%08x: rssb 0x%08x\n", i, vm->mem[i]);
}

word_t
rssb_vm_get_ptr(const rssb_vm_t *vm)
{
  return vm->mem_ptr;
}

void
rssb_vm_set_ptr(rssb_vm_t *vm, word_t ptr)
{
  vm->mem_ptr = ptr;
}

void
rssb_vm_set_dumb(rssb_vm_t *vm, BOOL dumb)
{
  vm->dumb_mode = dumb;
}

void
rssb_vm_destroy(rssb_vm_t *vm)
{
  if (vm->mem != NULL)
    free(vm->mem);

  free(vm);
}

rssb_vm_t *
rssb_vm_new(unsigned int size)
{
  rssb_vm_t *new = NULL;
  unsigned int mask = 1;

  TRYCATCH(size > RSSB_ADDR_MIN, goto fail);

  TRYCATCH(new = calloc(1, sizeof(rssb_vm_t)), goto fail);
  TRYCATCH(new->mem = calloc(size, sizeof(word_t)), goto fail);

  while (mask < size)
    mask <<= 1;

  new->mem_mask = mask - 1;
  new->mem_neg_mask = mask >> 1;
  new->mem_size = size;
  new->mem_ptr = RSSB_ADDR_MIN;
  new->mem[RSSB_ADDR_IP] = RSSB_ADDR_MIN;

  return new;

fail:
  if (new != NULL)
    rssb_vm_destroy(new);

  return NULL;
}

PRIVATE BOOL
rssb_vm_exec(rssb_vm_t *vm)
{
  word_t addr, word, acc, ip, result;
  BOOL skip;

  /* STEP 0: RETRIEVE INSTRUCTION */
  acc = vm->mem[RSSB_ADDR_A]  & vm->mem_mask;
  ip  = vm->mem[RSSB_ADDR_IP] & vm->mem_mask;
  if (ip >= vm->mem_size) {
    fprintf(stderr, "vm: invalid code address 0x%x\n", ip);
    return FALSE;
  }

  /* STEP 1: DECODE INSTRUCTION */
  addr = vm->mem[ip] & vm->mem_mask;
  if (addr >= vm->mem_size) {
    fprintf(stderr, "vm: invalid memory access to 0x%x at 0x%x\n", addr, ip);
    return FALSE;
  }

  /* STEP 2: RETRIEVE MEMORY */
  switch (addr) {
    case RSSB_ADDR_IN:
      word = getchar(); /* TODO: use input() */
      break;

    default:
      word = vm->mem[addr] & vm->mem_mask;
  }

#ifdef VM_DEBUG
  fprintf(
      stderr,
      "[%04x] rssb 0x%04x: %04x - %04x = ", ip, addr, word, acc);
#endif

  /* STEP 3: COMPUTE */
  if (vm->dumb_mode) {
    /* DUMB MODE: looks for result sign */
    if (addr == RSSB_ADDR_OUT)
      result = acc; /* Also what */
    else
      result = word - acc;

    skip = result & vm->mem_neg_mask;
  } else {
    /* RIGHT MODE: checks for borrow */
    skip = acc > word;
    result = word - acc;
  }

#ifdef VM_DEBUG
  fprintf(stderr, "%04x [%c]\n", result & vm->mem_mask, skip ? 'S' : ' ');
#endif

  /* STEP 4: UPDATE REGISTERS AND MEMORY */
  vm->mem[RSSB_ADDR_A] = result;

  if (addr == RSSB_ADDR_OUT)
    putchar(result);
  else if (addr != RSSB_ADDR_ZERO)
    vm->mem[addr] = vm->mem[RSSB_ADDR_A];

  /* STEP 5: Increment instruction pointer */
  vm->mem[RSSB_ADDR_IP] += 1 + !!skip;

  return TRUE;
}

BOOL
rssb_vm_run(rssb_vm_t *vm)
{
  /*
   * Exit procedure:
   *   rssb $ip # $a_1 = $ip - $a. $ip_1 = $a_1 + 1
   *   rssb $ip # $a_2 = ($a_1 + 1) - $a_1. $ip_2 = $a_2 + 1
   *
   *   $a_2  = 1
   *   $ip_2 = 2
   */
  while (vm->mem[RSSB_ADDR_IP] != 2 || vm->mem[RSSB_ADDR_A] != 1)
    if (!rssb_vm_exec(vm))
      return FALSE;

  return TRUE;
}
