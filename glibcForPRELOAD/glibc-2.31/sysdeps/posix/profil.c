/* Low-level statistical profiling support function.  Mostly POSIX.1 version.
   Copyright (C) 1996-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <libc-internal.h>
#include <sigsetops.h>
#include <time.h>

void *gmonparam_shadow = NULL;
u_long load_address = 0;
#include <sys/gmon.h>

#ifdef PIC
#include <link.h>
int callback (struct dl_phdr_info *info, size_t size, void *data)
{
  if (info->dlpi_name[0] == '\0')
  {
    u_long *load_address = data;
    *load_address = (u_long) info->dlpi_addr;
    return 1;
  }

  return 0;
}
#endif

#define HASHFRACTION 2
int pc2index(uintptr_t selfpc, struct gmonparam *p) {
  int j = -1;
  if ((HASHFRACTION & (HASHFRACTION - 1)) == 0) {
    j = (selfpc - p->lowpc) >> p->log_hashfraction;
  } else {
    j = (selfpc - p->lowpc) / (p->hashfraction * sizeof(*p->callsites));
  }
  if (selfpc - p->lowpc <= p->textsize)
    return j;
  return -1;
}

#ifndef SIGPROF

#include <gmon/profil.c>

#else

static u_short *samples;
static size_t nsamples;
static size_t pc_offset;
static u_int pc_scale;


#include "../generic/dwarf2.h"
const unsigned short reg_table_size = 16;
unsigned short const x86_64_regstr_tbl[] = {
  REG_RAX, REG_RDX, REG_RCX, REG_RBX, REG_RSI, REG_RDI,
  REG_RBP, REG_RSP, REG_R8, REG_R9, REG_R10, REG_R11,
  REG_R12, REG_R13, REG_R14, REG_R15, REG_RIP
};
static bool reg_valid[17] = {true};

long long readval(const void* bctx, unsigned short loc_atom, long addr, int size, void *b_read) {
  const ucontext_t *ctx = (const ucontext_t *)(bctx);
  bool *read = (bool *)b_read;
  long long val = 0xdeadbeef;
  switch(loc_atom) {
    case DW_OP_lit0:
    case DW_OP_lit1:
    case DW_OP_lit2:
    case DW_OP_lit3:
    case DW_OP_lit4:
    case DW_OP_lit5:
    case DW_OP_lit6:
    case DW_OP_lit7:
    case DW_OP_lit8:
    case DW_OP_lit9:
    case DW_OP_lit10:
    case DW_OP_lit11:
    case DW_OP_lit12:
    case DW_OP_lit13:
    case DW_OP_lit14:
    case DW_OP_lit15:
    case DW_OP_lit16:
    case DW_OP_lit17:
    case DW_OP_lit18:
    case DW_OP_lit19:
    case DW_OP_lit20:
    case DW_OP_lit21:
    case DW_OP_lit22:
    case DW_OP_lit23:
    case DW_OP_lit24:
    case DW_OP_lit25:
    case DW_OP_lit26:
    case DW_OP_lit27:
    case DW_OP_lit28:
    case DW_OP_lit29:
    case DW_OP_lit30:
    case DW_OP_lit31: {
                        val = (long long)(loc_atom - DW_OP_lit0);
                        *read = true;
                        break;
                      }
    case DW_OP_const1u:
    case DW_OP_const1s:
    case DW_OP_const2u:
    case DW_OP_const2s:
    case DW_OP_const4u:
    case DW_OP_const4s:
    case DW_OP_const8u:
    case DW_OP_const8s:
    case DW_OP_constu:
    case DW_OP_consts: {
                         val = (long long)addr;
                        *read = true;
                         break;
                       }
    case DW_OP_reg0:
    case DW_OP_reg1:
    case DW_OP_reg2:
    case DW_OP_reg3:
    case DW_OP_reg4:
    case DW_OP_reg5:
    case DW_OP_reg6:
    case DW_OP_reg7:
    case DW_OP_reg8:
    case DW_OP_reg9:
    case DW_OP_reg10:
    case DW_OP_reg11:
    case DW_OP_reg12:
    case DW_OP_reg13:
    case DW_OP_reg14:
    case DW_OP_reg15:
    case DW_OP_reg16:
    case DW_OP_reg17:
    case DW_OP_reg18:
    case DW_OP_reg19:
    case DW_OP_reg20:
    case DW_OP_reg21:
    case DW_OP_reg22:
    case DW_OP_reg23:
    case DW_OP_reg24:
    case DW_OP_reg25:
    case DW_OP_reg26:
    case DW_OP_reg27:
    case DW_OP_reg28:
    case DW_OP_reg29:
    case DW_OP_reg30:
    case DW_OP_reg31: {
                        //0x50 - 0x6f: register
                        unsigned short reg_id  = loc_atom - DW_OP_reg0;
                        if (reg_id < reg_table_size) {
                          val = (long long)(ctx->uc_mcontext.gregs[x86_64_regstr_tbl[reg_id]]);
                          *read = true;
                        }
                        break;
                      }
    case DW_OP_breg0:
    case DW_OP_breg1:
    case DW_OP_breg2:
    case DW_OP_breg3:
    case DW_OP_breg4:
    case DW_OP_breg5:
    case DW_OP_breg6:
    case DW_OP_breg7:
    case DW_OP_breg8:
    case DW_OP_breg9:
    case DW_OP_breg10:
    case DW_OP_breg11:
    case DW_OP_breg12:
    case DW_OP_breg13:
    case DW_OP_breg14:
    case DW_OP_breg15:
    case DW_OP_breg16:
    case DW_OP_breg17:
    case DW_OP_breg18:
    case DW_OP_breg19:
    case DW_OP_breg20:
    case DW_OP_breg21:
    case DW_OP_breg22:
    case DW_OP_breg23:
    case DW_OP_breg24:
    case DW_OP_breg25:
    case DW_OP_breg26:
    case DW_OP_breg27:
    case DW_OP_breg28:
    case DW_OP_breg29:
    case DW_OP_breg30:
    case DW_OP_breg31: {
                         //0x70 - 0x8f
                         unsigned short reg_id  = loc_atom - DW_OP_breg0;
                         if (reg_id < reg_table_size && reg_valid[reg_id] == true) {
                           long long *valptr = (long long *)(ctx->uc_mcontext.gregs[x86_64_regstr_tbl[reg_id]] + addr);
                           if (valptr != NULL && (u_long)valptr >= ctx->uc_mcontext.gregs[REG_RSP]) {
                             val = 0;
                             memcpy((void*)(&val), valptr, size);
                             *read = true;
                           }
                         }
                         break;
                       }

    case DW_OP_fbreg: {
                        long long* valptr = (long long *)(ctx->uc_mcontext.gregs[REG_RBP]
                            + 16 + addr);
                        if (valptr != NULL) {
                          val = 0;
                          memcpy((void*)(&val), valptr, size);
                          *read = true;
                        }
                        break;
                      }

    case DW_OP_addr: {
                       long long *valptr = (long long *)(addr + load_address);
                       if (valptr != NULL) {
                         val = 0;
                         memcpy((void*)(&val), valptr, size);
                         *read = true;
                       }
                       break;
                     }

    default:
                     val = 0xdeadbeef;
  }
  return val;
}

static void
sample_variables(struct gmonparam *p, const ucontext_t *ctx, long long timestamp,
    unsigned short sampled, u_long callee_pc) {

  u_long pc = ctx->uc_mcontext.gregs[REG_RIP] - load_address;
  int callsite_index = pc2index(ctx->uc_mcontext.gregs[REG_RIP], p);

  if (sampled >= UNWIND_PC) {
    pc = pc - 6; //adjust pc to the instruction before call
    callsite_index = pc2index(ctx->uc_mcontext.gregs[REG_RIP] - 6, p);
  } else {
    memset(reg_valid, true, reg_table_size * sizeof(bool));
  }
  
  int index = p->callsites[callsite_index];
  if (callsite_index < 0 
        || callsite_index >= p->fromssize / sizeof(ARCINDEX)
        || index == 0)
      return;

  while (index > 0 && index < p->tolimit) {
    if (pc < p->variables[index].lower_bound ||
        pc > p->variables[index].upper_bound) {
      goto next_var;
    }

    int size = p->variables[index].size;
    size = size >= 0 ? size : sizeof(void *);
    if (size > 8 || size == 0)
      goto next_var;

    bool read = false;
    long long val = readval(ctx, p->variables[index].loc_atom, p->variables[index].addr, size, &read);

    if (!read)
      goto next_var;

    //dereference if size is negative in config
    if (p->variables[index].size < 0) {
      if (val == 0) {
        val = 0xdeadcafe;
      } else {
        void *src = (void *)val;
        int size = (-p->variables[index].size) % 8;
        size = size == 0 ? 8 : size;
        val = 0;
        memcpy(&val, src, -p->variables[index].size);
      }
    }
    // store a new sample instance
    ARCINDEX* tail = &(p->variables[index].sample_tail);
    ARCINDEX val_index = ++p->samples[0].link;
    if (val_index >= p->samplelimit) {
      return;
    }
    struct samplestruct *top = &p->samples[val_index];
    top->seq_id = timestamp;
    top->sampled = sampled;
    top->val = val;
    top->tid = gettid();
    top->link = *tail;
    top->var_pc = pc;//ctx->uc_mcontext.gregs[REG_RIP] - 6;
    top->cur_pc = callee_pc;
    *tail = val_index;

next_var:
    //process the next variable instrumented for the same pc
    index = p->variables[index].link;
  }
}
#if UNWINDCALLER
#if IS_IN (libc)
#define UNW_LOCAL_ONLY
#include <libunwind.h>
static int store_context(ucontext_t *dest, unw_cursor_t *cursor)
{
  //save register values from cursor
  int ret = 0;
  unw_word_t reg;
  for (int i = 0; i < reg_table_size; ++i) {
    unsigned short id = x86_64_regstr_tbl[i];
    ret = unw_get_reg(cursor, i, &reg);
    if (ret == UNW_ESUCCESS) {
        dest->uc_mcontext.gregs[id] = (long long int)reg;
        reg_valid[i] = true;
    } else {
        reg_valid[i] = false;
    }
  }
  //update rip
  ret = unw_get_reg(cursor, UNW_REG_IP, &reg);
  dest->uc_mcontext.gregs[x86_64_regstr_tbl[reg_table_size]] = reg;
  return ret;
}
#if FIX_OUTRANGE_PC_GPROF
static int sample_record(size_t pc) {
  size_t i = (pc - pc_offset) / 2;
  if (sizeof (unsigned long long int) > sizeof (size_t))
    i = (unsigned long long int) i * pc_scale / 65536;
  else
    i = i / 65536 * pc_scale + i % 65536 * pc_scale / 65536;
  return i;
}
#endif

static void unwind_sample(struct gmonparam *p, const ucontext_t *ctx,
  long long timestamp, const int max_depth) {
  
  //save current context
  ucontext_t prev_ctx;
  memcpy(&prev_ctx, ctx, sizeof(ucontext_t));
  
  //init unwind context
  unw_context_t context;
  unw_cursor_t cursor;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  //skip current frame, sample frame (profil_count), signal handler frame and callee frame in ctx.
  unw_word_t ip;
  while (unw_step(&cursor) > 0) {
    store_context(&prev_ctx, &cursor);
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    if (ip == ctx->uc_mcontext.gregs[REG_RIP])  //callee ip
      break;
  }
  if (ip != ctx->uc_mcontext.gregs[REG_RIP] || unw_step(&cursor) <= 0)
    return;
#if FIX_OUTRANGE_PC_GPROF
  int sample_index = sample_record(ip);
#endif
  //get previous max_depth and sample variables with context
  for (int i = 0; i < max_depth; ++i) {
    store_context(&prev_ctx, &cursor);
    sample_variables(p, &prev_ctx, timestamp, UNWIND_PC + i, ip - load_address);
#if FIX_OUTRANGE_PC_GPROF
    if (sample_index >= nsamples) {
      sample_index = sample_record(prev_ctx.uc_mcontext.gregs[REG_RIP]);
      if (sample_index < nsamples)
        ++samples[sample_index];
    }
#endif
    if (unw_step(&cursor) <= 0)
      break;
  }
}
#else
static void unwind_sample(struct gmonparam *p, const ucontext_t *ctx,
  long long timestamp, const int max_depth) {
}
#endif
#endif

static int launch_pid = 0;
static bool set_itimer = false;
int check_timer(void)
{
  if (set_itimer == true || __getpid() == launch_pid) {
    return 0;
  }
#ifdef PIC
  __dl_iterate_phdr (callback, &load_address);
#endif
  struct itimerval timer;
  static struct itimerval otimer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 1000000 / __profile_frequency ();
  timer.it_interval = timer.it_value;
  int ret = __setitimer (ITIMER_PROF, &timer, &otimer);
  if (timer.it_interval.tv_sec != otimer.it_interval.tv_sec
      || timer.it_interval.tv_usec != otimer.it_interval.tv_usec) {
    set_itimer = true;
  }
  return ret;
}

static inline void
profil_count (uintptr_t pc, const void *ctx, uintptr_t fp)
{
  size_t i = (pc - pc_offset) / 2;

  if (sizeof (unsigned long long int) > sizeof (size_t))
    i = (unsigned long long int) i * pc_scale / 65536;
  else
    i = i / 65536 * pc_scale + i % 65536 * pc_scale / 65536;

  if (i < nsamples)
    ++samples[i];

  check_timer();

  if (gmonparam_shadow == NULL)
	  return;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct gmonparam *p = (struct gmonparam *)gmonparam_shadow;
  if (p->variables == NULL 
      || p->samples[0].link < 0
      || p->samples[0].link >= p->samplelimit)
	  return;
  
  sample_variables(p, (const ucontext_t *)ctx, 
      tv.tv_sec * 1000000 + tv.tv_usec, i < nsamples ? 1 : 0, pc - load_address);
  
#if UNWINDCALLER
  //even though current pc is not within the monitored range
  //we still check its callers. 
  //libunwind to sample caller variables
  const int depth = 3;
  unwind_sample(p, (const ucontext_t *)ctx, tv.tv_sec * 1000000 + tv.tv_usec, depth);
#endif
}


/* Get the machine-dependent definition of `__profil_counter', the signal
   handler for SIGPROF.  It calls `profil_count' (above) with the PC of the
   interrupted code.  */
#include "profil-counter.h"

/* Enable statistical profiling, writing samples of the PC into at most
   SIZE bytes of SAMPLE_BUFFER; every processor clock tick while profiling
   is enabled, the system examines the user PC and increments
   SAMPLE_BUFFER[((PC - OFFSET) / 2) * SCALE / 65536].  If SCALE is zero,
   disable profiling.  Returns zero on success, -1 on error.  */

int
__profil (u_short *sample_buffer, size_t size, size_t offset, u_int scale)
{
  struct sigaction act;
  struct itimerval timer;
#if !IS_IN (rtld)
  static struct sigaction oact;
  static struct itimerval otimer;
# define oact_ptr &oact
# define otimer_ptr &otimer

  if (sample_buffer == NULL)
  {
    /* Disable profiling.  */
    if (samples == NULL) {
      /* Wasn't turned on.  */
      return 0;
    }

    if (__setitimer (ITIMER_PROF, &otimer, NULL) < 0) {
      return -1;
    }
    samples = NULL;
    return __sigaction (SIGPROF, &oact, NULL);
  }

  if (samples)
  {
    /* Was already turned on.  Restore old timer and signal handler
       first.  */
    if (__setitimer (ITIMER_PROF, &otimer, NULL) < 0
        || __sigaction (SIGPROF, &oact, NULL) < 0) {
      return -1;
    }
  }
#else
 /* In ld.so profiling should never be disabled once it runs.  */
 //assert (sample_buffer != NULL);
# define oact_ptr NULL
# define otimer_ptr NULL
#endif

  samples = sample_buffer;
  nsamples = size / sizeof *samples;
  pc_offset = offset;
  pc_scale = scale;

#ifdef SA_SIGINFO
  act.sa_sigaction = __profil_counter;
  act.sa_flags = SA_SIGINFO;
#else
  act.sa_handler = __profil_counter;
  act.sa_flags = 0;
#endif
  act.sa_flags |= SA_RESTART;
  __sigfillset (&act.sa_mask);
  if (__sigaction (SIGPROF, &act, oact_ptr) < 0) {
    return -1;
  }

#ifdef PIC
  __dl_iterate_phdr (callback, &load_address);
#endif

  launch_pid = __getpid();
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 1000000 / __profile_frequency ();
  timer.it_interval = timer.it_value;
  return __setitimer (ITIMER_PROF, &timer, otimer_ptr);
}
weak_alias (__profil, profil)

#endif
