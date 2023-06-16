/* Superoptimizer.  Finds the shortest instruction sequences for an arbitrary
   function y=f(x) where x is a vector of integer and y is a is a single
   integers.  The algorithm is based on exhaustive search with backtracking
   and iterative deepening.

   Copyright (C) 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; see the file COPYING.  If not, write to the Free
   Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <stdio.h>
#include <string.h>

#include "superopt.h"

word random_word(void);

#include "run_program.def"
#include "version.h"

int goal_function_arity;
enum goal_func goal_function;
word (*eval_goal_function) (const word *);

int flag_output_assembly = 0;
int flag_use_carry = 1;
int flag_shifts = 0;
int flag_extracts = 0;
int flag_nl = 0;

/* Counts the number of solutions found.  Flags to top loop that it should
   not go deeper.  */
int success;

#ifdef TIMING
#ifndef USG
#include <sys/time.h>
#include <sys/resource.h>

unsigned long
cputime ()
{
    struct rusage rus;

    getrusage (0, &rus);
    return rus.ru_utime.tv_sec * 1000 + rus.ru_utime.tv_usec / 1000;
}
#else
#include <time.h>

unsigned long
cputime ()
{
  return clock () / 1000;
}
#endif
int timings[100];
#endif

#ifdef STATISTICS
unsigned int heuristic_reject_count = 0;
unsigned int heuristic_accept_count = 0;
#endif

char *insn_name[] =
{
#undef	DEF_INSN
#define DEF_INSN(SYM,CLASS,NAME) NAME,
#include "insn.def"
};

char insn_class[] =
{
#undef	DEF_INSN
#define DEF_INSN(SYM,CLASS,NAME) CLASS,
#include "insn.def"
};

/* Initialize the "immediate" values in the top registers.  */
void
init_immediates(word *values)
{
  int i;
  for (i = -1; i < BITS_PER_WORD; i++)
    values[0x20 + i] = i;

  values[0x20 - 2] = VALUE_MIN_SIGNED;
  values[0x20 - 3] = VALUE_MAX_SIGNED;
  values[0x20 - 4] = 0xFFFF;
  values[0x20 - 5] = 0xFF;
}

#if defined (__svr4) || defined (__hpux) || defined (_SVR4_SOURCE)
#define random mrand48
#endif

void
init_random_word (void)
{
#if defined (__svr4) || defined (__hpux) || defined (_SVR4_SOURCE)
  static unsigned short state1[3] = { 0xeb3d, 0x799f, 0xb11e };
  srand48 (state1);
#else
  static char state1[128] =
    {
      0, 0, 0, 3,
      0x9a,0x31,0x90,0x39, 0x32,0xd9,0xc0,0x24, 0x9b,0x66,0x31,0x82, 0x5d,0xa1,0xf3,0x42,
      0x74,0x49,0xe5,0x6b, 0xbe,0xb1,0xdb,0xb0, 0xab,0x5c,0x59,0x18, 0x94,0x65,0x54,0xfd,
      0x8c,0x2e,0x68,0x0f, 0xeb,0x3d,0x79,0x9f, 0xb1,0x1e,0xe0,0xb7, 0x2d,0x43,0x6b,0x86,
      0xda,0x67,0x2e,0x2a, 0x15,0x88,0xca,0x88, 0xe3,0x69,0x73,0x5d, 0x90,0x4f,0x35,0xf7,
      0xd7,0x15,0x8f,0xd6, 0x6f,0xa6,0xf0,0x51, 0x61,0x6e,0x6b,0x96, 0xac,0x94,0xef,0xdc,
      0xde,0x3b,0x81,0xe0, 0xdf,0x0a,0x6f,0xb5, 0xf1,0x03,0xbc,0x02, 0x48,0xf3,0x40,0xfb,
      0x36,0x41,0x3f,0x93, 0xc6,0x22,0xc2,0x98, 0xf5,0xa4,0x2a,0xb8, 0x8a,0x88,0xd7,0x7b,
      0xf5,0xad,0x9d,0x0e, 0x89,0x99,0x22,0x0b, 0x27,0xfb,0x47,0xb9
    };
  unsigned seed;
  int n;

  seed = 1;
  n = 128;
  initstate (seed, (char *) state1, n);
  setstate (state1);
#endif
}

#define RANDOM(res,rbits) \
  do {								\
    if (valid_randbits < rbits)					\
      {								\
	ran = random ();					\
	valid_randbits = 31;					\
      }								\
    res = ran & (((word) 1 << rbits) - 1);			\
    ran >>= rbits;						\
    valid_randbits -= rbits;					\
  } while (0)

word
random_word (void)
{
  word x;
  unsigned int ran, n_bits;
  int valid_randbits = 0;
  int tmp, i;

  /* Return small values with higher probability.  */
  RANDOM (tmp, 9);
  if (tmp <= 60)
    return tmp + 4;

  /* Start off with only ones or only zeros, with equal probability.  */
  RANDOM (tmp, 1);
  x = 0;
  if (tmp)
    x = ~x;

  /* Now generate random strings of ones and zeros.  */
  n_bits = 0;
  for (i = 8 * sizeof (word); i >= 0; i -= n_bits)
    {
      RANDOM (tmp, 3);
      n_bits = tmp + 1;
      x <<= n_bits;
      RANDOM (tmp, 1);
      if (tmp)
	x |= ((word) 1 << n_bits) - 1;
    }

  return x;
}

void *malloc (), *realloc ();

char *
xrealloc (ptr, size)
     char *ptr;
     unsigned size;
{
  char *result = (char *) realloc (ptr, size);
  if (!result)
    abort ();
  return result;
}

char *
xmalloc (size)
     unsigned size;
{
  register char *val = (char *) malloc (size);

  if (val == 0)
    abort ();
  return val;
}

#if HAS_NULLIFICATION
#define SYNTH(sequence, n_insns, values, n_values, goal_value, \
	      allowed_cost, cy, prune_flags, nullify_flag)		\
  do {									\
    if (allowed_cost == 1)						\
      synth_last(sequence, n_insns, values, n_values, goal_value,	\
		 allowed_cost, cy, prune_flags, nullify_flag);		\
    else								\
      synth(sequence, n_insns, values, n_values, goal_value,		\
	    allowed_cost, cy, prune_flags, nullify_flag);		\
  } while (0)
#else
#define SYNTH(sequence, n_insns, values, n_values, goal_value, \
	      allowed_cost, cy, prune_flags, nullify_flag)		\
  do {									\
    if (allowed_cost == 1)						\
      synth_last(sequence, n_insns, values, n_values, goal_value,	\
		 allowed_cost, cy, prune_flags);			\
    else								\
      synth(sequence, n_insns, values, n_values, goal_value,		\
	    allowed_cost, cy, prune_flags);				\
  } while (0)
#endif

/* Save the last generated instruction and recursively call `synth', if we
   are not at a leaf node.  Otherwise test the computed value and
   back-track.  This function is extremely critical for the performance!

   OPCODE is the opcode of the insn that was just generated.

   D is the destination register.

   S1 is the left source register or immediate reference.  It is an
   immediate reference if IMMEDIATE_P(S1) is true.

   S2 is the right source register or immediate reference.  It is an
   immediate reference if IMMEDIATE_P(S2) is true.

   V is the computed result from "rD = rS1 OPCODE rS2".

   COST is the cost of OPCODE with the the actual operands.

   SEQUENCE is the insn sequence so far, excluding the just generated insn.

   N_INSNS is the number of insns in SEQUENCE.

   VALUES contains the values in register 0..N_VALUES.

   N_VALUES is the number of registers that have been assigned values by
   the insns so far.

   GOAL_VALUE is the value we aim at, when the sequence is ready.

   ALLOWED_COST is the maximum allowed cost of the remaining sequence.

   CY is the carry flag.  It is negative if it has an undefined value (this
   for pruning the search tree), and otherwise takes the values 0 or 1
   according to the conventions of the current target.

   PRUNE_HINT contains flags to assist pruning of the search tree.  */

static void
recurse(opcode_t opcode,
	int d,
	int s1,
	int s2,
	word v,
	int cost,
	insn_t *sequence,
	int n_insns,
	word *values,
	int n_values,
	const word goal_value,
	int allowed_cost,
	int cy,
	int prune_flags
#if HAS_NULLIFICATION
	,int nullify_flag
#endif
	)
{
  insn_t insn;

  /* Update the remaining allowed cost with the cost of the last
     instruction.  */
  allowed_cost -= cost;

  if (allowed_cost > 0)
    {
      /* ALLOWED_COST is still positive, meaning we can generate more
	 instructions.  */
      word old_d;

#if HAS_NULLIFICATION
      /* If we we have one more instruction, and it will be nullified for the
	 used arguments, ensure already now that we have the goal value
	 somewhere.  Kludge for speed.  */
      if (allowed_cost == 1 && nullify_flag)
	{
	  int i;
	  if (v == goal_value)
	    goto found_goal_value;
	  for (i = 0; i < n_values; i++)
	    if (values[i] == goal_value)
	      goto found_goal_value;
	  return;
	found_goal_value:;
	}
#endif

      /* Remember old value of dest. reg.  Move to CRECURSE_2OP???  */
      old_d = values[d];
      values[d] = v;

#if __GNUC__
      sequence[n_insns] = (insn_t) {opcode, s1, s2, d};
#else
      insn.opcode = opcode;
      insn.s1 = s1;
      insn.s2 = s2;
      insn.d = d;
      sequence[n_insns] = insn;
#endif

      SYNTH(sequence, n_insns + 1, values, n_values, goal_value,
	     allowed_cost, cy, prune_flags, nullify_flag);

      /* Restore value of dest. reg.  Move to CRECURSE_2OP???  */
      values[d] = old_d;
    }
  else if (goal_value == v)
    {
      /* We are at a leaf node and got the right answer for the
	 random value operands.  However, we probably have an
	 incorrect sequence.  Call test_sequence to find out.  */

#if __GNUC__
      sequence[n_insns] = (insn_t) {opcode, s1, s2, d};
#else
      insn.opcode = opcode;
      insn.s1 = s1;
      insn.s2 = s2;
      insn.d = d;
      sequence[n_insns] = insn;
#endif
      test_sequence(sequence, n_insns + 1);

#ifdef STATISTICS
      heuristic_accept_count++;
#endif

    }
#ifdef STATISTICS
  else
    heuristic_reject_count++;
#endif
}

static inline void
recurse_last(opcode_t opcode,
	     int d,
	     int s1,
	     int s2,
	     word v,
	     insn_t *sequence,
	     int n_insns,
	     const word goal_value)
{
  insn_t insn;

  if (goal_value == v)
    {
      /* We are at a leaf node and got the right answer for the
	 random value operands.  However, we probably have an
	 incorrect sequence.  Call test_sequence to find out.  */

#if __GNUC__
      sequence[n_insns] = (insn_t) {opcode, s1, s2, d};
#else
      insn.opcode = opcode;
      insn.s1 = s1;
      insn.s2 = s2;
      insn.d = d;
      sequence[n_insns] = insn;
#endif
      test_sequence(sequence, n_insns + 1);

#ifdef STATISTICS
      heuristic_accept_count++;
#endif
    }
#ifdef STATISTICS
  else
    heuristic_reject_count++;
#endif
}

#define NAME(op) operand_names[op]
static char *operand_names[256]=
{
#if SPARC
  "%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7",
#elif POWER
  "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
#elif M88000
  "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
#elif AM29K
  "lr2", "lr3", "lr4", "lr5", "lr6", "lr7", "lr8", "lr9",
#elif M68000
  "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
#elif I386
  "%eax", "%edx", "%ecx", "%ebx", "%esi", "%edi", "%noooo!", "%crash!!!",
#elif PYR
  "pr0", "pr1", "pr2", "pr3", "pr4", "pr5", "pr6", "pr7",
#elif ALPHA
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
#elif HPPA
  "%r26", "%r25", "%r24", "%r23", "%r22", "%r21", "%r20", "%r19",
#elif SH
  "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
#elif I960
  "g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7",
#else
#error no register names for this CPU
#endif
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
#if SPARC
  "???%hi(0x7fffffff)","%hi(0x80000000)","-1","%g0","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
#elif POWER
  "0x7fff","0x8000","-1","0","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
#elif M88000
  "hi16(0x7fffffff)","hi16(0x80000000)","-1","r0","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
#elif AM29K
  "0x7fff","0x8000","-1","0","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
#elif M68000 || SH
  "#0x7fffffff","#0x80000000","#-1","#0","#1","#2","#3","#4","#5","#6",
  "#7","#8","#9","#10","#11","#12","#13","#14","#15","#16","#17","#18","#19",
  "#20","#21","#22","#23","#24","#25","#26","#27","#28","#29","#30","#31",
#elif I386 || PYR
  "$0x7fffffff","$0x80000000","$-1","$0","$1","$2","$3","$4","$5","$6",
  "$7","$8","$9","$10","$11","$12","$13","$14","$15","$16","$17","$18","$19",
  "$20","$21","$22","$23","$24","$25","$26","$27","$28","$29","$30","$31",
#elif ALPHA
  "0x7fff","0x8000","-1","$31","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
  "32","33","34","35","36","37","38","39","40","41","42","43",
  "44","45","46","47","48","49","50","51","52","53","54","55",
  "56","57","58","59","60","61","62","63",
#elif HPPA
  "0x7fff","0x8000","-1","%r0","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
#elif I960
  "0x7fffffff","0x80000000","-1","0","1","2","3","4","5","6",
  "7","8","9","10","11","12","13","14","15","16","17","18","19",
  "20","21","22","23","24","25","26","27","28","29","30","31",
#else
#error no constant syntax for this CPU
#endif
};

/* Output INSN in assembly mnemonic format on stdout.  */

void
output_assembly(insn_t insn)
{
  int d, s1, s2;

  d = insn.d;
  s1 = insn.s1;
  s2 = insn.s2;

  printf("\t");
  switch (insn.opcode)
    {
#if SPARC
    case COPY:
      if (IMMEDIATE_P(s1) && (IMMEDIATE_VAL(s1) & 0x1fff) == 0)
	printf("sethi	%s,%s",NAME(s1),NAME(d));
      else
	printf("mov	%s,%s",NAME(s1),NAME(d));
      break;
    case ADD:	printf("add	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ADD_CI:printf("addx	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ADD_CO:printf("addcc	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ADD_CIO:printf("addxcc	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("xnor	%%g0,%s,%s",NAME(s2),NAME(d));
      else
	printf("sub	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_CI:printf("subx	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case SUB_CO:printf("subcc	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case SUB_CIO:printf("subxcc	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case AND:	printf("and	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case IOR:	printf("or	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case XOR:	printf("xor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ANDC:	printf("andn	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case IORC:	printf("orn	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case EQV:	printf("xnor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case LSHIFTR:printf("srl	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ASHIFTR:printf("sra	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case SHIFTL:printf("sll	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
#elif POWER
#ifdef POWERPC			/* redefine the PowerPC names */
#define INS_LIL		"li"
#define INS_LIU		"li"
#define INS_ORIL	"ori"
#define INS_CAL		"addi"
#define	INS_CAU		"addis"
#define	INS_CAX		"add"
#define	INS_AI		"addic"
#define	INS_A		"addc"
#define	INS_AME		"addme"
#define	INS_AZE		"aze"
#define	INS_AE		"adde"
#define	INS_NEG		"neg"
#define	INS_SF		"subfc"
#define	INS_SUBF	"subf"
#define	INS_SFI		"subfic"
#define	INS_SFME	"subfme"
#define	INS_SFZE	"subfze"
#define	INS_SFE		"subfe"
#define	INS_AND		"and"
#define	INS_ORIU	"oris"
#define	INS_ORIL	"ori"
#define	INS_OR		"or"
#define	INS_XORIL	"xori"
#define	INS_XORIU	"xoris"
#define	INS_XOR		"xor"
#define	INS_ANDC	"andc"
#define	INS_ORC		"orc"
#define	INS_EQV		"eqv"
#define	INS_NAND	"nand"
#define	INS_NOR		"nor"
#define	INS_SRI		"srwi"
#define	INS_SRE		"srw"
#define	INS_SRAI	"srawi"
#define	INS_SREA	"sraw"
#define	INS_SLI		"slwi"
#define	INS_SLE		"slw"
#define	INS_RLINM	"rlwimi"
#define	INS_RLNM	"rlwnm"
#define	INS_MULI	"mulli"
#define	INS_MULS	"mullw"
#define	INS_CNTLZ	"cntlzw"
#define	INS_ABS		"abs (illegal)"
#define	INS_NABS	"nabs (illegal)"
#define	INS_DOZI	"dozi (illegal)"
#define	INS_DOZ		"doz (illegal)"

#else  /* Power names, not PowerPC */
#define INS_LIL		"lil"
#define INS_LIU		"liu"
#define INS_ORIL	"oril"
#define INS_CAL		"cal"
#define	INS_CAU		"cau"
#define	INS_CAX		"cax"
#define	INS_AI		"ai"
#define	INS_A		"a"
#define	INS_AME		"ame"
#define	INS_AZE		"aze"
#define	INS_AE		"ae"
#define	INS_NEG		"neg"
#define	INS_SF		"sf"
#define	INS_SUBF	"subf (illegal)"
#define	INS_SFI		"sfi"
#define	INS_SFME	"sfme"
#define	INS_SFZE	"sfze"
#define	INS_SFE		"sfe"
#define	INS_AND		"and"
#define	INS_ORIU	"oriu"
#define	INS_ORIL	"oril"
#define	INS_OR		"or"
#define	INS_XORIL	"xoril"
#define	INS_XORIU	"xoriu"
#define	INS_XOR		"xor"
#define	INS_ANDC	"andc"
#define	INS_ORC		"orc"
#define	INS_EQV		"eqv"
#define	INS_NAND	"nand"
#define	INS_NOR		"nor"
#define	INS_SRI		"sri"
#define	INS_SRE		"sre"
#define	INS_SRAI	"srai"
#define	INS_SREA	"srea"
#define	INS_SLI		"sli"
#define	INS_SLE		"sle"
#define	INS_RLINM	"rlinm"
#define	INS_RLNM	"rlnm"
#define	INS_MULI	"muli"
#define	INS_MULS	"muls"
#define	INS_CNTLZ	"cntlz"
#define	INS_ABS		"abs"
#define	INS_NABS	"nabs"
#define	INS_DOZI	"dozi"
#define	INS_DOZ		"doz"
#endif /* POWER */

    case COPY:
      if (IMMEDIATE_P(s1))
	{
	  if (IMMEDIATE_VAL(s1) >= 0 && IMMEDIATE_VAL(s1) < 0x8000)
	    printf("%s\t%s,0x%x",INS_LIL,NAME(d),IMMEDIATE_VAL(s1));
	  else
	    printf("%s\t%s,%s",INS_LIU,NAME(d),NAME(s1));
	}
      else
	printf("%s\t%s,%s,0",INS_ORIL,NAME(d),NAME(s1));
      break;
    case ADD:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) + 0x4000 < 0x8000)
	    printf("%s\t%s,%d(%s)",INS_CAL,NAME(d),IMMEDIATE_VAL(s2),NAME(s1));
	  else if ((IMMEDIATE_VAL(s2) & 0xffff) == 0)
	    printf("%s\t%s,%s,0x%x",INS_CAU,NAME(d),NAME(s1),IMMEDIATE_VAL(s2) >> 16);
	  else
	    abort ();
	}
      else
	printf("%s\t%s,%s,%s",INS_CAX,NAME(d),NAME(s1),NAME(s2));
      break;
    case ADD_CO:
      if (IMMEDIATE_P(s2))
	printf("%s\t%s,%s,%s",INS_AI,NAME(d),NAME(s1),NAME(s2));
      else
	printf("%s\t%s,%s,%s",INS_A,NAME(d),NAME(s1),NAME(s2));
      break;
    case ADD_CIO:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == -1)
	    printf("%s\t%s,%s",INS_AME,NAME(d),NAME(s1));
	  else if (IMMEDIATE_VAL(s2) == 0)
	    printf("%s\t%s,%s",INS_AZE,NAME(d),NAME(s1));
	}
      else
	printf("%s\t%s,%s,%s",INS_AE,NAME(d),NAME(s1),NAME(s2));
      break;
    case SUB:
      if (IMMEDIATE_P(s1))
	{
	  if (IMMEDIATE_VAL(s1) == 0)
	    printf("%s\t%s,%s",INS_NEG,NAME(d),NAME(s2));
	  else if (IMMEDIATE_VAL(s1) == -1)
	    printf("%s\t%s,%s,%s",INS_NAND,NAME(d),NAME(s2),NAME(s2));
	}
      else
	printf("%s\t%s,%s,%s",INS_SUBF,NAME(d),NAME(s2),NAME(s1));
      break;
    case ADC_CO:
      if (IMMEDIATE_P(s1))
	printf("%s\t%s,%s,%s",INS_SFI,NAME(d),NAME(s2),NAME(s1));
      else
	printf("%s\t%s,%s,%s",INS_SF,NAME(d),NAME(s2),NAME(s1));
      break;
    case ADC_CIO:
      if (IMMEDIATE_P(s1))
	{
	  if (IMMEDIATE_VAL(s1) == -1)
	    printf("%s\t%s,%s",INS_SFME,NAME(d),NAME(s2));
	  else if (IMMEDIATE_VAL(s1) == 0)
	    printf("%s\t%s,%s",INS_SFZE,NAME(d),NAME(s2));
	  else abort();
	}
      else
	printf("%s\t%s,%s,%s",INS_SFE,NAME(d),NAME(s2),NAME(s1));
      break;
    case AND:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == 0x80000000)
	    printf("%s\t%s,%s,0,0,0",INS_RLINM,NAME(d),NAME(s1));
	  else if (IMMEDIATE_VAL(s2) == 0x7fffffff)
	    printf("%s\t%s,%s,0,1,31",INS_RLINM,NAME(d),NAME(s1));
	  else if (IMMEDIATE_VAL(s2) == 1)
	    printf("%s\t%s,%s,0,31,31",INS_RLINM,NAME(d),NAME(s1));
	  else abort();
	}
      else
	printf("%s\t%s,%s,%s",INS_AND,NAME(d),NAME(s1),NAME(s2));
      break;
    case IOR:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == 0x80000000)
	    printf("%s\t%s,%s,0x8000",INS_ORIU,NAME(d),NAME(s1));
	  else abort();
	}
      else
	printf("%s\t%s,%s,%s",INS_OR,NAME(d),NAME(s1),NAME(s2));
      break;
    case XOR:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == 1)
	    printf("%s\t%s,%s,1",INS_XORIL,NAME(d),NAME(s1));
	  else if (IMMEDIATE_VAL(s2) == 0x80000000)
	    printf("%s\t%s,%s,0x8000",INS_XORIU,NAME(d),NAME(s1));
	  else abort();
	}
      else
	printf("%s\t%s,%s,%s",INS_XOR,NAME(d),NAME(s1),NAME(s2));
      break;
    case ANDC:	printf("%s\t%s,%s,%s",INS_ANDC,NAME(d),NAME(s1),NAME(s2));break;
    case IORC:	printf("%s\t%s,%s,%s",INS_ORC,NAME(d),NAME(s1),NAME(s2));break;
    case EQV:	printf("%s\t%s,%s,%s",INS_EQV,NAME(d),NAME(s1),NAME(s2));break;
    case NAND:	printf("%s\t%s,%s,%s",INS_NAND,NAME(d),NAME(s1),NAME(s2));break;
    case NOR:	printf("%s\t%s,%s,%s",INS_NOR,NAME(d),NAME(s1),NAME(s2));break;
    case LSHIFTR:
      if (IMMEDIATE_P(s2))
	printf("%s\t%s,%s,%s",INS_SRI,NAME(d),NAME(s1),NAME(s2));
      else
	printf("%s\t%s,%s,%s",INS_SRE,NAME(d),NAME(s1),NAME(s2));
      break;
    case ASHIFTR_CON:
      if (IMMEDIATE_P(s2))
	printf("%s\t%s,%s,%s",INS_SRAI,NAME(d),NAME(s1),NAME(s2));
      else
	printf("%s\t%s,%s,%s",INS_SREA,NAME(d),NAME(s1),NAME(s2));
      break;
    case SHIFTL:
      if (IMMEDIATE_P(s2))
	printf("%s\t%s,%s,%s",INS_SLI,NAME(d),NAME(s1),NAME(s2));
      else
	printf("%s\t%s,%s,%s",INS_SLE,NAME(d),NAME(s1),NAME(s2));
      break;
    case ROTATEL:
      if (IMMEDIATE_P(s2))
	printf("%s\t%s,%s,%s,0,31",INS_RLINM,NAME(d),NAME(s1),NAME(s2));
      else
	printf("%s\t%s,%s,%s,0,31",INS_RLNM,NAME(d),NAME(s1),NAME(s2));
      break;
#ifndef POWERPC
    case ABSVAL:printf("%s\t%s,%s",INS_ABS,NAME(d),NAME(s1));break;
    case NABSVAL:printf("%s\t%s,%s",INS_NABS,NAME(d),NAME(s1));break;
    case DOZ:
      if (IMMEDIATE_P(s1))
	printf("%s\t%s,%s,%s",INS_DOZI,NAME(d),NAME(s2),NAME(s1));
      else
	printf("%s\t%s,%s,%s",INS_DOZ,NAME(d),NAME(s2),NAME(s1));
      break;
#endif
    case MUL:
      if (IMMEDIATE_P(s1))
	printf("%s\t%s,%s,%s",INS_MULI,NAME(d),NAME(s2),NAME(s1));
      else
	printf("%s\t%s,%s,%s",INS_MULS,NAME(d),NAME(s2),NAME(s1));
      break;
    case CLZ:
      printf("%s\t%s,%s",INS_CNTLZ,NAME(d),NAME(s1));break;
#elif M88000
    case COPY:
      if (IMMEDIATE_P(s1))
	{
	  if ((IMMEDIATE_VAL(s1) & 0xffff) == 0)
	    printf("or.u	%s,r0,0x%x",NAME(d),IMMEDIATE_VAL(s1));
	  else if ((IMMEDIATE_VAL(s1) & 0xffff0000) == 0)
	    printf("or	%s,r0,0x%x",NAME(d),IMMEDIATE_VAL(s1));
	  else if ((IMMEDIATE_VAL(s1) & 0xffff0000) == 0xffff0000)
	    printf("subu	%s,r0,0x%x",NAME(d),-IMMEDIATE_VAL(s1));
	  else
	    {
	      word x = IMMEDIATE_VAL(s1);
	      int i, j;
	      for (i = 31; i > 0; i--)
		if ((x & (1 << i)) != 0)
		  break;
	      for (j = i; j >= 0; j--)
		if ((x & (1 << j)) == 0)
		  break;
	      printf("set	%s,r0,%d<%d>",NAME(d), i - j, j + 1);
	    }
	}
      else
	printf("or	%s,%s",NAME(d),NAME(s1));
      break;
    case ADD:	printf("addu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ADD_CI:printf("addu.ci	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ADD_CO:printf("addu.co	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ADD_CIO:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == -1)
	    {
	      printf("subu.cio %s,%s,r0",NAME(d),NAME(s1));
	      break;
	    }
	  if (IMMEDIATE_VAL(s2) != 0)
	    abort();
	}
      printf("addu.cio %s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("xor.c	%s,%s,r0",NAME(d),NAME(s2));
      else
	printf("subu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case ADC_CI:printf("subu.ci	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ADC_CO:printf("subu.co	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ADC_CIO:printf("subu.cio %s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case AND:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == 0x80000000)
	    printf("mask.u	%s,%s,0x8000",NAME(d),NAME(s1));
	  else if (IMMEDIATE_VAL(s2) == 0x7fffffff)
	    printf("and.u	%s,%s,0x7fff",NAME(d),NAME(s1));
	  else if (IMMEDIATE_VAL(s2) == 1)
	    printf("mask	%s,%s,1",NAME(d),NAME(s1));
	  else abort();
	}
      else
	printf("and	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case IOR:
      if (IMMEDIATE_P(s2))
	{
	  if ((IMMEDIATE_VAL(s2) & 0xffff) == 0)
	    printf("or.u	%s,%s,0x%x",NAME(d),NAME(s1),
		   IMMEDIATE_VAL(s2)>>16);
	  else if (IMMEDIATE_VAL(s2) < 0x10000)
	    printf("or	%s,%s,0x%x",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
	  else abort();
	}
      else
	printf("or	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case XOR:
      if (IMMEDIATE_P(s2))
	{
	  if ((IMMEDIATE_VAL(s2) & 0xffff) == 0)
	    printf("xor.u	%s,%s,0x%x",NAME(d),NAME(s1),
		   IMMEDIATE_VAL(s2)>>16);
	  else if (IMMEDIATE_VAL(s2) < 0x10000)
	    printf("xor	%s,%s,0x%x",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
	  else abort();
	}
      else
	printf("xor	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case ANDC:	printf("and.c	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case IORC:	printf("or.c	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case EQV:	printf("xor.c	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case LSHIFTR:printf("extu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ASHIFTR:printf("ext	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case SHIFTL:printf("mak	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ROTATEL:
      printf("rot	%s,%s,%d",NAME(d),NAME(s1),32-IMMEDIATE_VAL(s2));
      break;
    case FF1:
      printf("ff1	%s,%s",NAME(d),NAME(s1));break;
    case FF0:
      printf("ff0	%s,%s",NAME(d),NAME(s1));break;
    case CMPPAR:
      if (IMMEDIATE_P(s2))
	printf("cmp	%s,%s,0x%x",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
      else if (IMMEDIATE_P(s1))
	printf("cmp	%s,r0,%s",NAME(d),NAME(s2));
      else
	printf("cmp	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case EXTS1:
      printf("ext	%s,%s,1<%d>",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
      break;
    case EXTS2:
      printf("ext	%s,%s,2<%d>",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
      break;
    case EXTU1:
      printf("extu	%s,%s,1<%d>",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
      break;
    case EXTU2:
      printf("extu	%s,%s,2<%d>",NAME(d),NAME(s1),IMMEDIATE_VAL(s2));
      break;
    case MUL:
      printf("mul	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
#elif AM29K
    case COPY:
      if (IMMEDIATE_P(s1))
	{
	  if (IMMEDIATE_VAL(s1) < 0x10000)
	    printf("const	%s,0x%x",NAME(d),IMMEDIATE_VAL(s1));
	  else if (-IMMEDIATE_VAL(s1) < 0x10000)
	    printf("constn	%s,-0x%x",NAME(d),-IMMEDIATE_VAL(s1));
	  else if (IMMEDIATE_VAL(s1) == 0x80000000)
	    printf("cpeq	%s,gr1,gr1",NAME(d));
	  else abort();
	}
      else
	printf("or	%s,%s,0",NAME(d),NAME(s1));
      break;
    case ADD_CO:
      if (IMMEDIATE_P(s2) && (signed_word) IMMEDIATE_VAL(s2) < 0)
	printf("sub	%s,%s,0x%x",NAME(d),NAME(s1),-IMMEDIATE_VAL(s2));
      else
	printf("add	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case ADD_CIO:
      if (IMMEDIATE_P(s2) && (signed_word) IMMEDIATE_VAL(s2) < 0)
	printf("subc	%s,%s,0x%x",NAME(d),NAME(s1),-IMMEDIATE_VAL(s2));
      else
	printf("addc	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("nor	%s,%s,0",NAME(d),NAME(s2));
      else abort();
      break;
    case ADC_CO:printf("sub	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ADC_CIO:printf("subc	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case AND:	printf("and	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case IOR:	printf("or	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case XOR:	printf("xor	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ANDC:	printf("andn	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case EQV:	printf("xnor	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case NAND:	printf("nand	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case NOR:	printf("nor	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case LSHIFTR:printf("srl	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case ASHIFTR:printf("sra	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case SHIFTL:printf("sll	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPEQ:	printf("cpeq	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPGE:	printf("cpge	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPGEU:	printf("cpgeu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPGT:	printf("cpgt	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPGTU:	printf("cpgtu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPLE:	printf("cple	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPLEU:	printf("cpleu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPLT:	printf("cplt	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPLTU:	printf("cpltu	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case CPNEQ:	printf("cpneq	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));break;
    case MUL:
      printf("multiply	%s,%s,%s",NAME(d),NAME(s1),NAME(s2));
      break;
    case CLZ:
      printf("clz	%s,%s",NAME(d),NAME(s1));break;
#elif M68000
    case COPY:
      if (IMMEDIATE_P(s1))
	{
	  if ((signed_word) IMMEDIATE_VAL(s1) >= -128
	      && (signed_word) IMMEDIATE_VAL(s1) < 128)
	    {
	      printf("moveq	#%ld,%s", IMMEDIATE_VAL(s1),NAME(d));
	      break;
	    }
	}
      printf("movel	%s,%s",NAME(s1),NAME(d));
      break;
    case EXCHANGE:
      printf("exgl	%s,%s",NAME(s2),NAME(d));break;
    case ADD_CO:
      if (IMMEDIATE_P(s2)
	  && IMMEDIATE_VAL(s2) >= 1 && IMMEDIATE_VAL(s2) <= 8)
	printf("addql	%s,%s",NAME(s2),NAME(d));
      else
	printf("addl	%s,%s",NAME(s2),NAME(d));
      break;
    case ADD_CIO:
      printf("addxl	%s,%s",NAME(s2),NAME(d));break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("notl	%s",NAME(d));
      else abort();
      break;
    case SUB_CO:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == 0)
	printf("negl	%s",NAME(d));
      else if (IMMEDIATE_P(s2)
	       && IMMEDIATE_VAL(s2) >= 1 && IMMEDIATE_VAL(s2) <= 8)
	printf("subql	%s,%s",NAME(s2),NAME(d));
      else
	printf("subl	%s,%s",NAME(s2),NAME(d));
      break;
    case SUB_CIO:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == 0)
	printf("negxl	%s",NAME(d));
      else
	printf("subxl	%s,%s",NAME(s2),NAME(d));
      break;
    case AND:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) + 0x8000 < 0x10000)
	printf("andw	%s,%s",NAME(s2),NAME(d));
      else
	printf("andl	%s,%s",NAME(s2),NAME(d));
      break;
    case IOR:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) + 0x8000 < 0x10000)
	printf("orw	%s,%s",NAME(s2),NAME(d));
      else
	printf("orl	%s,%s",NAME(s2),NAME(d));
      break;
    case XOR:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) + 0x8000 < 0x10000)
	printf("eorw	%s,%s",NAME(s2),NAME(d));
      else
	printf("eorl	%s,%s",NAME(s2),NAME(d));
      break;
    case LSHIFTR_CO:
      printf("lsrl	%s,%s",NAME(s2),NAME(d));break;
    case ASHIFTR_CO:
      printf("asrl	%s,%s",NAME(s2),NAME(d));break;
    case SHIFTL_CO:
      printf("lsll	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEL_CO:
      printf("roll	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEXL_CIO:
      printf("roxll	%s,%s",NAME(s2),NAME(d));break;
    case ROTATER_CO:
      printf("rorl	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEXR_CIO:
      printf("roxrl	%s,%s",NAME(s2),NAME(d));break;
    case MUL:
      printf("mulsl	%s,%s",NAME(s2),NAME(d));break;
#elif I386
    case COPY:
      printf("movl	%s,%s",NAME(s1),NAME(d));break;
    case BSF86:
      printf("bsfl	%s,%s",NAME(s1),NAME(d));break;
    case EXCHANGE:
      printf("xchgl	%s,%s",NAME(s2),NAME(d));break;
    case ADD:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == 1)
	    printf("incl	%s",NAME(d));
	  else if (IMMEDIATE_VAL(s2) == -1)
	    printf("decl	%s",NAME(d));
	  else abort();
	}
      else abort();
      break;
    case ADD_CO:
      printf("addl	%s,%s",NAME(s2),NAME(d));break;
    case ADD_CIO:
      printf("adcl	%s,%s",NAME(s2),NAME(d));break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("notl	%s",NAME(d));
      else abort();
      break;
    case SUB_CO:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == 0)
	printf("negl	%s",NAME(d));
      else
	printf("subl	%s,%s",NAME(s2),NAME(d));
      break;
    case SUB_CIO:
      printf("sbbl	%s,%s",NAME(s2),NAME(d));break;
    case CMP:
      printf("cmpl	%s,%s",NAME(s2),NAME(s1));break;
    case AND_RC:
      if (IMMEDIATE_P(s2) && (IMMEDIATE_VAL(s2) & 0xffffff00) == 0xffffff00)
	printf("andb	$%d,%s",IMMEDIATE_VAL(s2),NAME(d));
      else
	printf("andl	%s,%s",NAME(s2),NAME(d));
      break;
    case IOR_RC:
      if (IMMEDIATE_P(s2) && (IMMEDIATE_VAL(s2) & 0xffffff00) == 0)
	printf("orb	$%d,%s",IMMEDIATE_VAL(s2),NAME(d));
      else
	printf("orl	%s,%s",NAME(s2),NAME(d));
      break;
    case XOR_RC:
      if (IMMEDIATE_P(s2) && (IMMEDIATE_VAL(s2) & 0xffffff00) == 0)
	printf("xorb	$%d,%s",IMMEDIATE_VAL(s2),NAME(d));
      else
	printf("xorl	%s,%s",NAME(s2),NAME(d));
      break;
    case LSHIFTR_CO:
      printf("shrl	%s,%s",NAME(s2),NAME(d));break;
    case ASHIFTR_CO:
      printf("sarl	%s,%s",NAME(s2),NAME(d));break;
    case SHIFTL_CO:
      printf("shll	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEL_CO:
      printf("roll	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEXL_CIO:
      printf("rlcl	%s,%s",NAME(s2),NAME(d));break;
    case ROTATER_CO:
      printf("rorl	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEXR_CIO:
      printf("rrcl	%s,%s",NAME(s2),NAME(d));break;
    case COMCY:
      printf("cmc");break;
    case MUL:
      printf("imull	%s,%s",NAME(s2),NAME(d));break;
#elif PYR
    case COPY:
      printf("movw	%s,%s",NAME(s1),NAME(d));break;
    case EXCHANGE:
      printf("xchw	%s,%s",NAME(s2),NAME(d));break;
    case ADD:
      printf("mova	0x%x(%s),%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      break;
    case ADD_CO:
      printf("addw	%s,%s",NAME(s2),NAME(d));break;
    case ADD_CIO:
      printf("addwc	%s,%s",NAME(s2),NAME(d));break;
    case ADC_CO:
      if (IMMEDIATE_P(s1))
	printf("rsubw	%s,%s",NAME(s1),NAME(d));
      else
	printf("subw	%s,%s",NAME(s2),NAME(d));
      break;
    case ADC_CIO:
      printf("subwb	%s,%s",NAME(s2),NAME(d));break;
    case AND_CC:
      printf("andw	%s,%s",NAME(s2),NAME(d));break;
    case IOR_CC:
      printf("orw	%s,%s",NAME(s2),NAME(d));break;
    case XOR_CC:
      printf("xorw	%s,%s",NAME(s2),NAME(d));break;
    case ANDC_CC:
      printf("bicw	%s,%s",NAME(s2),NAME(d));break;
    case LSHIFTR_CO:
      printf("lshrw	%s,%s",NAME(s2),NAME(d));break;
    case ASHIFTR_CO:
      printf("ashrw	%s,%s",NAME(s2),NAME(d));break;
    case SHIFTL_CO:
      printf("lshlw	%s,%s",NAME(s2),NAME(d));break;
    case ROTATEL_CO:
      printf("rotlw	%s,%s",NAME(s2),NAME(d));break;
    case ROTATER_CO:
      printf("rotrw	%s,%s",NAME(s2),NAME(d));break;
    case MUL:
      printf("mulw	%s,%s",NAME(s2),NAME(d));break;
#elif ALPHA
    case COPY:
      if (IMMEDIATE_P(s1))
	printf("lda	%s,%s",NAME(d),NAME(s1)); /* yes, reversed op order */
      else
	printf("bis	%s,%s,%s",NAME(s1),NAME(s1),NAME(d));
      break;
    case ADD:
      if (IMMEDIATE_P(s2) && (signed_word) IMMEDIATE_VAL(s2) < 0)
	/* Cast value to int since we cannot portably print a 64 bit type.  */
	printf("subq	%s,%d,%s",NAME(s1),(int) -IMMEDIATE_VAL(s2),NAME(d));
      else
	printf("addq	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("ornot	$31,%s,%s",NAME(s2),NAME(d));
      else
	printf("subq	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND:	printf("and	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case IOR:	printf("bis	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case XOR:	printf("xor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ANDC:	printf("bic	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case IORC:	printf("ornot	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case EQV:	printf("eqv	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case LSHIFTR:printf("srl	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ASHIFTR:printf("sra	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case SHIFTL:printf("sll	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMPEQ:	printf("cmpeq	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMPLE:	printf("cmple	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMPLEU:printf("cmpule	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMPLT:	printf("cmplt	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMPLTU:printf("cmpult	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMOVEQ:printf("cmoveq	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMOVNE:printf("cmovne	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMOVLT:printf("cmovlt	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMOVGE:printf("cmovge	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMOVLE:printf("cmovle	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case CMOVGT:printf("cmovgt	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
#elif HPPA
    case ADD_CIO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO:
      printf("subb		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO:
      if (IMMEDIATE_P(s2))
	printf("addi		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO:
      if (IMMEDIATE_P(s1))
	printf("subi		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD:
      if (IMMEDIATE_P(s2))
	printf("ldo		%d(%s),%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("addl		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADDCMPL:
      printf("uaddcm		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND:
      if (IMMEDIATE_P(s2))
	printf("extru		%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR:
      printf("or		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR:
      printf("xor		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC:
      printf("andcm		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB:
      printf("andcm		%%r0,%s,%s",NAME(s2),NAME(d));
      break;
    case LSHIFTR:
      printf("extru		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR:
      printf("extrs		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL:
      printf("zdep		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL:
      printf("shd		%s,%s,%d,%s",NAME(s1),NAME(s1),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTS1:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extrs		%s,%d,1,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTU1:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extru		%s,%d,1,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTS2:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extrs		%s,%d,2,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTU2:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extru		%s,%d,2,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case COPY:
      if (IMMEDIATE_P(s1))
	{
	  if (IMMEDIATE_VAL(s1) + 0x2000 < 0x4000)
	    printf("ldi		%d,%s",IMMEDIATE_VAL(s1),NAME(d));
	  else if ((IMMEDIATE_VAL(s1) & 0x7ff) == 0)
	    printf("ldil		l'0x%x,%s",IMMEDIATE_VAL(s1),NAME(d));
	  else if (IMMEDIATE_VAL(s1) == 0x7fffffff)
	    printf("zdepi		-1,31,31,%s",NAME(d));
	  else
	    abort();
	}
      else
	printf("copy		%s,%s",NAME(s1),NAME(d));
      break;
    case ADD_CIO_SEQ:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,=		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SEQ:
      printf("subb,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SEQ:
      if (IMMEDIATE_P(s2))
	printf("addi,=		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SEQ:
      if (IMMEDIATE_P(s1))
	printf("subi,=		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SEQ:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SEQ:
      if (IMMEDIATE_P(s1))
	printf("comiclr,=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SEQ:
      if (IMMEDIATE_P(s2))
	printf("extru,=		%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SEQ:
      printf("or,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SEQ:
      printf("xor,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SEQ:
      printf("andcm,=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SEQ:
      printf("andcm,=		%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SEQ:
      printf("extru,=		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SEQ:
      printf("extrs,=		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SEQ:
      printf("zdep,=		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SEQ:
    case EXTS1_SEQ:
    case EXTU1_SEQ:
    case EXTS2_SEQ:
    case EXTU2_SEQ:
      abort();
    case COPY_SEQ:
      abort();
#endif

    case ADD_CIO_SNE:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,<>		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SNE:
      printf("subb,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SNE:
      if (IMMEDIATE_P(s2))
	printf("addi,<>		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SNE:
      if (IMMEDIATE_P(s1))
	printf("subi,<>		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SNE:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SNE:
      if (IMMEDIATE_P(s1))
	printf("comiclr,<>	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,<>	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SNE:
      if (IMMEDIATE_P(s2))
	printf("extru,<>	%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SNE:
      printf("or,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SNE:
      printf("xor,<>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SNE:
      printf("andcm,<>	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SNE:
      printf("andcm,<>	%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SNE:
      printf("extru,<>	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SNE:
      printf("extrs,<>	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SNE:
      printf("zdep,<>		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SNE:
    case EXTS1_SNE:
    case EXTU1_SNE:
    case EXTS2_SNE:
    case EXTU2_SNE:
      abort();
    case COPY_SNE:
      abort();
#endif

#if LATER
    case ADD_CIO_SLTS:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,<		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SLTS:
      printf("subb,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SLTS:
      if (IMMEDIATE_P(s2))
	printf("addi,<		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SLTS:
      if (IMMEDIATE_P(s1))
	printf("subi,<		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
#endif
    case ADD_SLTS:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SLTS:
      if (IMMEDIATE_P(s1))
	printf("comiclr,<	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,<	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SLTS:
      if (IMMEDIATE_P(s2))
	printf("extru,<		%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SLTS:
      printf("or,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SLTS:
      printf("xor,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SLTS:
      printf("andcm,<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SLTS:
      printf("andcm,<		%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SLTS:
      printf("extru,<		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SLTS:
      printf("extrs,<		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SLTS:
      printf("zdep,<		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SLTS:
    case EXTS1_SLTS:
    case EXTU1_SLTS:
    case EXTS2_SLTS:
    case EXTU2_SLTS:
      abort();
    case COPY_SLTS:
      abort();
#endif

#if LATER
    case ADD_CIO_SGES:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,>=		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SGES:
      printf("subb,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SGES:
      if (IMMEDIATE_P(s2))
	printf("addi,>=		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SGES:
      if (IMMEDIATE_P(s1))
	printf("subi,>=		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
#endif
    case ADD_SGES:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SGES:
      if (IMMEDIATE_P(s1))
	printf("comiclr,>=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,>=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SGES:
      if (IMMEDIATE_P(s2))
	printf("extru,>=	%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SGES:
      printf("or,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SGES:
      printf("xor,>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SGES:
      printf("andcm,>=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SGES:
      printf("andcm,>=	%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SGES:
      printf("extru,>=	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SGES:
      printf("extrs,>=	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SGES:
      printf("zdep,>=		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SGES:
    case EXTS1_SGES:
    case EXTU1_SGES:
    case EXTS2_SGES:
    case EXTU2_SGES:
      abort();
    case COPY_SGES:
      abort();
#endif

#if LATER
    case ADD_CIO_SLES:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,<=		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SLES:
      printf("subb,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SLES:
      if (IMMEDIATE_P(s2))
	printf("addi,<=		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SLES:
      if (IMMEDIATE_P(s1))
	printf("subi,<=		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
#endif
    case ADD_SLES:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SLES:
      if (IMMEDIATE_P(s1))
	printf("comiclr,<=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,<=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SLES:
      if (IMMEDIATE_P(s2))
	printf("extru,<=	%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SLES:
      printf("or,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SLES:
      printf("xor,<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SLES:
      printf("andcm,<=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SLES:
      printf("andcm,<=	%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SLES:
      printf("extru,<=	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SLES:
      printf("extrs,<=	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SLES:
      printf("zdep,<=		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SLES:
    case EXTS1_SLES:
    case EXTU1_SLES:
    case EXTS2_SLES:
    case EXTU2_SLES:
      abort();
    case COPY_SLES:
      abort();
#endif

#if LATER
    case ADD_CIO_SGTS:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,>		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SGTS:
      printf("subb,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SGTS:
      if (IMMEDIATE_P(s2))
	printf("addi,>		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SGTS:
      if (IMMEDIATE_P(s1))
	printf("subi,>		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
#endif
    case ADD_SGTS:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SGTS:
      if (IMMEDIATE_P(s1))
	printf("comiclr,>	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,>	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SGTS:
      if (IMMEDIATE_P(s2))
	printf("extru,>		%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SGTS:
      printf("or,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SGTS:
      printf("xor,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SGTS:
      printf("andcm,>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SGTS:
      printf("andcm,>		%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SGTS:
      printf("extru,>		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SGTS:
      printf("extrs,>		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SGTS:
      printf("zdep,>		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SGTS:
    case EXTS1_SGTS:
    case EXTU1_SGTS:
    case EXTS2_SGTS:
    case EXTU2_SGTS:
      abort();
    case COPY_SGTS:
      abort();
#endif

    case ADD_CIO_SLTU:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,<<		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,nuv	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SLTU:
      printf("subb,<<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SLTU:
      if (IMMEDIATE_P(s2))
	printf("addi,nuv	%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,nuv		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SLTU:
      if (IMMEDIATE_P(s1))
	printf("subi,<<		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,<<		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SLTU:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,nuv	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SLTU:
      if (IMMEDIATE_P(s1))
	printf("comiclr,<<	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,<<	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;

    case ADD_CIO_SGEU:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,>>=	%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,uv		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SGEU:
      printf("subb,>>=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SGEU:
      if (IMMEDIATE_P(s2))
	printf("addi,uv		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,uv		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SGEU:
      if (IMMEDIATE_P(s1))
	printf("subi,>>=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,>>=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SGEU:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,uv		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SGEU:
      if (IMMEDIATE_P(s1))
	printf("comiclr,>>=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,>>=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;

    case ADD_CIO_SLEU:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,<<=	%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,znv	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SLEU:
      printf("subb,<<=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SLEU:
      if (IMMEDIATE_P(s2))
	printf("addi,znv	%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,znv		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SLEU:
      if (IMMEDIATE_P(s1))
	printf("subi,<<=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,<<=		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SLEU:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,znv	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SLEU:
      if (IMMEDIATE_P(s1))
	printf("comiclr,<<=	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,<<=	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;

    case ADD_CIO_SGTU:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,>>		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,vnz	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SGTU:
      printf("subb,>>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SGTU:
      if (IMMEDIATE_P(s2))
	printf("addi,vnz	%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,vnz		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SGTU:
      if (IMMEDIATE_P(s1))
	printf("subi,>>		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,>>		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SGTU:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,vnz	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SGTU:
      if (IMMEDIATE_P(s1))
	printf("comiclr,>>	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,>>	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;

    case ADD_CIO_SODD:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,od		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SODD:
      printf("subb,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SODD:
      if (IMMEDIATE_P(s2))
	printf("addi,od		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SODD:
      if (IMMEDIATE_P(s1))
	printf("subi,od		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SODD:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SODD:
      if (IMMEDIATE_P(s1))
	printf("comiclr,od	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,od	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SODD:
      if (IMMEDIATE_P(s2))
	printf("extru,od	%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SODD:
      printf("or,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SODD:
      printf("xor,od		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SODD:
      printf("andcm,od	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SODD:
      printf("andcm,od	%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SODD:
      printf("extru,od	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SODD:
      printf("extrs,od	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SODD:
      printf("zdep,od		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SODD:
    case EXTS1_SODD:
    case EXTU1_SODD:
    case EXTS2_SODD:
    case EXTU2_SODD:
      abort();
    case COPY_SODD:
      abort();
#endif

    case ADD_CIO_SEVN:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,ev		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_SEVN:
      printf("subb,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_SEVN:
      if (IMMEDIATE_P(s2))
	printf("addi,ev		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_SEVN:
      if (IMMEDIATE_P(s1))
	printf("subi,ev		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SEVN:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case COMCLR_SEVN:
      if (IMMEDIATE_P(s1))
	printf("comiclr,ev	%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("comclr,ev	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_SEVN:
      if (IMMEDIATE_P(s2))
	printf("extru,ev	%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_SEVN:
      printf("or,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_SEVN:
      printf("xor,ev		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_SEVN:
      printf("andcm,ev	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_SEVN:
      printf("andcm,ev	%%r0,%s,%s",NAME(s2),NAME(d));
      break;
#if LATER
    case LSHIFTR_SEVN:
      printf("extru,ev	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_SEVN:
      printf("extrs,ev	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_SEVN:
      printf("zdep,ev		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_SEVN:
    case EXTS1_SEVN:
    case EXTU1_SEVN:
    case EXTS2_SEVN:
    case EXTU2_SEVN:
      abort();
    case COPY_SEVN:
      abort();
#endif

    case ADD_SOVS:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,sv		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_SNVS:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,nsv	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;

    case ADD_CIO_S:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == -1)
	printf("subb,tr		%s,%%r0,%s",NAME(s1),NAME(d));
      else
	printf("addc,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CIO_S:
      printf("subb,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_CO_S:
      if (IMMEDIATE_P(s2))
	printf("addi,tr		%d,%s,%s",IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("add,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADC_CO_S:
      if (IMMEDIATE_P(s1))
	printf("subi,tr		%d,%s,%s",IMMEDIATE_VAL(s1),NAME(s2),NAME(d));
      else
	printf("sub,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ADD_S:
      if (IMMEDIATE_P(s2))
	abort();
      else
	printf("addl,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case AND_S:
      if (IMMEDIATE_P(s2))
	printf("extru,tr	%s,31,%d,%s",NAME(s1),ffs_internal(IMMEDIATE_VAL(s2) + 1) - 1,NAME(d));
      else
	printf("and,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case IOR_S:
      printf("or,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case XOR_S:
      printf("xor,tr		%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case ANDC_S:
      printf("andcm,tr	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB_S:
      printf("andcm,tr	%%r0,%s,%s",NAME(s2),NAME(d));
      break;
    case LSHIFTR_S:
      printf("extru,tr	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ASHIFTR_S:
      printf("extrs,tr	%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case SHIFTL_S:
      printf("zdep,tr		%s,%d,%d,%s",NAME(s1),31-IMMEDIATE_VAL(s2),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case ROTATEL_S:
      printf("shd,tr		%s,%s,%d,%s",NAME(s1),NAME(s1),32-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTS1_S:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extrs,tr	%s,%d,1,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTU1_S:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extru,tr	%s,%d,1,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTS2_S:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extrs,tr	%s,%d,2,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case EXTU2_S:
      if (!IMMEDIATE_P(s2))
	abort();
      printf("extru,tr	%s,%d,2,%s",NAME(s1),31-IMMEDIATE_VAL(s2),NAME(d));
      break;
    case COPY_S:
      if (IMMEDIATE_P(s1))
	{
	  if (IMMEDIATE_VAL(s1) + 0x400 < 0x800)
	    printf("addi,tr		%d,%%r0,%s",IMMEDIATE_VAL(s1),NAME(d));
	  else if (IMMEDIATE_VAL(s1) == 0x7fffffff)
	    printf("zdepi,tr	-1,31,31,%s",NAME(d));
	  else if (IMMEDIATE_VAL(s1) == 0x80000000)
	    printf("zdepi,tr	1,0,1,%s",NAME(d));
	  else
	    abort();
	}
      else
	printf("addl,tr		%s,%%r0,%s",NAME(s1),NAME(d));
      break;
#elif SH
    case COPY:
      printf("mov	%s,%s",NAME(s1),NAME(d));break;
    case ADD:
      printf("add	%s,%s",NAME(s2),NAME(d));break;
    case ADD_CI:
      printf("movt	%s",NAME(d));break;
    case ADD_CIO:
      printf("addc	%s,%s",NAME(s2),NAME(d));break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == 0)
	printf("neg	%s,%s",NAME(s2),NAME(d));
      else
	printf("sub	%s,%s",NAME(s2),NAME(d));
      break;
    case SUB_CIO:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == 0)
	printf("negc	%s,%s",NAME(s2),NAME(d));
      else
	printf("subc	%s,%s",NAME(s2),NAME(d));
      break;
    case AND:
      if (IMMEDIATE_P(s2))
	{
	  if (IMMEDIATE_VAL(s2) == 0xff)
	    printf("extu.b	%s,%s",NAME(s1),NAME(d));
	  else if (IMMEDIATE_VAL(s2) == 0xffff)
	    printf("extu.w	%s,%s",NAME(s1),NAME(d));
	  else
	    printf("and	%s,%s	! reallocate %s in r0",NAME(s2),NAME(d),NAME(d));
	  break;
	}
      printf("and	%s,%s",NAME(s2),NAME(d));
      break;
    case IOR:
      if (IMMEDIATE_P(s2))
	{
	  printf("or	%s,%s	! reallocate %s in r0",NAME(s2),NAME(d),NAME(d));
	  break;
	}
      printf("or	%s,%s",NAME(s2),NAME(d));break;
    case XOR:
      if (IMMEDIATE_P(s2))
	{
	  printf("xor	%s,%s	! reallocate %s in r0",NAME(s2),NAME(d),NAME(d));
	  break;
	}
      printf("xor	%s,%s",NAME(s2),NAME(d));break;
    case SHIFTL_CO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("shll	%s",NAME(d));
      else abort();
      break;
    case SHIFTL:
      if (IMMEDIATE_P(s2)
	  && (IMMEDIATE_VAL(s2) == 2 || IMMEDIATE_VAL(s2) == 8 || IMMEDIATE_VAL(s2) == 16))
	printf("shll%d	%s",IMMEDIATE_VAL(s2),NAME(d));
      else abort();
      break;

    case LSHIFTR_CO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("shlr	%s",NAME(d));
      else abort();
      break;
    case LSHIFTR:
      if (IMMEDIATE_P(s2)
	  && (IMMEDIATE_VAL(s2) == 2 || IMMEDIATE_VAL(s2) == 8 || IMMEDIATE_VAL(s2) == 16))
	printf("shlr%d	%s",IMMEDIATE_VAL(s2),NAME(d));
      else abort();
      break;

    case ASHIFTR_CO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("shar	%s",NAME(d));
      else abort();
      break;
    case ROTATEL_CO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("rotl	%s",NAME(d));
      else abort();
      break;
    case ROTATEL:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 16)
	printf("swap.w	%s,%s",NAME(s1),NAME(d));
      else abort();
      break;
    case MERGE16:
      printf("xtrct	%s,%s",NAME(s2),NAME(d));
      break;
    case ROTATEXL_CIO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("rotcl	%s",NAME(d));
      else abort();
      break;
    case ROTATER_CO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("rotr	%s",NAME(d));
      else abort();
      break;
    case ROTATEXR_CIO:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 1)
	printf("rotcr	%s",NAME(d));
      else abort();
      break;
    case CYEQ:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 0)
	printf("tst	%s,%s",NAME(s1),NAME(s1));
      else
	printf("cmp/eq	%s,%s",NAME(s2),NAME(s1));
      break;
    case CYGTS:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 0)
	printf("cmp/pl	%s",NAME(s1));
      else
	printf("cmp/gt	%s,%s",NAME(s2),NAME(s1));
      break;
    case CYGES:
      if (IMMEDIATE_P(s2) && IMMEDIATE_VAL(s2) == 0)
	printf("cmp/pz	%s",NAME(s1));
      else
	printf("cmp/ge	%s,%s",NAME(s2),NAME(s1));
      break;
    case CYGTU:
      printf("cmp/hi	%s,%s",NAME(s2),NAME(s1));break;
    case CYGEU:
      printf("cmp/hs	%s,%s",NAME(s2),NAME(s1));break;
    case CYAND:
      if (IMMEDIATE_P(s2))
	printf("tst	%s,%s	! reallocate %s in r0",NAME(s2),NAME(s1),NAME(s1));
      else
	printf("tst	%s,%s",NAME(s2),NAME(s1));
      break;
    case SETCY:
      printf("sett");break;
    case CLRCY:
      printf("clrt");break;
    case EXTS8:
      printf("exts.b	%s,%s",NAME(s1),NAME(d));break;
    case EXTS16:
      printf("exts.w	%s,%s",NAME(s1),NAME(d));break;
    case DECR_CYEQ:
      printf("dt	%s",NAME(d));break;
#elif I960
    case ADD:
      if (IMMEDIATE_P(s2) && (signed_word) IMMEDIATE_VAL(s2) < 0)
	printf("subo	%d,%s,%s",-IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("addo	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUB:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) == -1)
	printf("not	%s,%s",NAME(s2),NAME(d));
      else
	printf("subo	%s,%s,%s",NAME(s2),NAME(s1),NAME(d));
      break;
    case COPY:
      if (IMMEDIATE_P(s1) && IMMEDIATE_VAL(s1) >= 32)
	{
	  word x = IMMEDIATE_VAL(s1);
	  if ((x & (x - 1)) == 0)
	    {
	      printf("setbit	%d,1,%s",floor_log2(x),NAME(d));
	      break;
	    }
	  if (-x < 32)
	    {
	      printf("sub	%d,%s",-x,NAME(d));
	      break;
	    }
	  abort();
	}
      printf("mov	%s,%s",NAME(s1),NAME(d));break;
    case AND:
      printf("and	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case IOR:
      printf("or	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case IORC:
      printf("notor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case XOR:
      printf("xor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case ANDC:
      printf("notand	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case EQV:
      printf("xnor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case NAND:
      printf("nand	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case NOR:
      printf("nor	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));break;
    case LSHIFTR:
      printf("shro	%s,%s,%s",NAME(s2),NAME(s1),NAME(d));break;
    case ASHIFTR:
      printf("shri	%s,%s,%s",NAME(s2),NAME(s1),NAME(d));break;
    case SHIFTL:
      printf("shlo	%s,%s,%s",NAME(s2),NAME(s1),NAME(d));break;
    case ROTATEL:
      printf("rotate	%s,%s,%s",NAME(s2),NAME(s1),NAME(d));break;
    case ADDC_960:
      if (IMMEDIATE_P(s2) && (signed_word) IMMEDIATE_VAL(s2) < 0)
	printf("subc	%d,%s,%s",-IMMEDIATE_VAL(s2),NAME(s1),NAME(d));
      else
	printf("addc	%s,%s,%s",NAME(s1),NAME(s2),NAME(d));
      break;
    case SUBC_960:
      printf("subc	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SEL_NO_960:
      printf("selno	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_G_960:
      printf("selg	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_E_960:
      printf("sele	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_GE_960:
      printf("selge	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_L_960:
      printf("sell	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_NE_960:
      printf("selne	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_LE_960:
      printf("selle	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SEL_O_960:
      printf("selo	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case CONCMPO_960:
      printf("concmpo	%s,%s",NAME(s1),NAME(s2)); break;
    case CONCMPI_960:
      printf("concmpi	%s,%s",NAME(s1),NAME(s2)); break;
    case CMPO_960:
      printf("cmpo	%s,%s",NAME(s1),NAME(s2)); break;
    case CMPI_960:
      printf("cmpi	%s,%s",NAME(s1),NAME(s2)); break;
    case SHIFTL_NT:
      printf("shlo	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case LSHIFTR_NT:
      printf("shro	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case ASHIFTR_NT:
      printf("shri	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case ADDO_NO_960:
      printf("addono	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_G_960:
      printf("addog	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_E_960:
      printf("addoe	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_GE_960:
      printf("addoge	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_L_960:
      printf("addol	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_NE_960:
      printf("addone	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_LE_960:
      printf("addole	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case ADDO_O_960:
      printf("addoo	%s,%s,%s",NAME(s1),NAME(s2),NAME(d)); break;
    case SUBO_NO_960:
      printf("subono	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_G_960:
      printf("subog	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_E_960:
      printf("suboe	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_GE_960:
      printf("suboge	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_L_960:
      printf("subol	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_NE_960:
      printf("subone	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_LE_960:
      printf("subole	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SUBO_O_960:
      printf("suboo	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case ALTERBIT:
      printf("alterbit	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case SETBIT:
      printf("setbit	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case CLRBIT:
      printf("clrbit	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
    case CHKBIT:
      printf("chkbit	%s,%s",NAME(s2),NAME(s1)); break;
    case NOTBIT:
      printf("notbit	%s,%s,%s",NAME(s2),NAME(s1),NAME(d)); break;
#else
#error no assembly output code for this CPU
#endif
    default:abort();
    }
  printf("\n");
}

word tvalues[0x100];

/* TEST_SETS contains sets of input values and corresponding goal value used
   to test the correctness of a sequence.  N_TEST_SETS says how many sets
   are defined.  */
word *test_sets;
int n_test_sets;

word test_operands[] =
{
  -3, -2, -1, 0, 1, 2, 3, 30, 31, 32, 63, 64,
  VALUE_MIN_SIGNED,
  VALUE_MAX_SIGNED,
  VALUE_MIN_SIGNED + 1,
  VALUE_MAX_SIGNED - 1,
};
#define N_TEST_OPERANDS (sizeof(test_operands)/sizeof(test_operands[0]))

#ifndef N_RANDOM_TEST_OPERANDS
#define N_RANDOM_TEST_OPERANDS 25000
#endif

void
init_test_sets()
{
  unsigned int loop_vars[8];
  int pc, i, j;
  word *test_set;
  const int arity = goal_function_arity;
  word (*eval) (const word *) = eval_goal_function;

  if (sizeof (loop_vars) / sizeof (loop_vars[0]) <= arity)
    abort ();

  /* Allocate enough space in TEST_SETS for all combinations of TEST_OPERANDS
     and an additional N_RANDOM_TEST_OPERANDS random test sets.  */
  {
    static int n_words = 0;

    j = 1 + arity;
    for (i = arity - 1; i >= 0; i--)
      j *= N_TEST_OPERANDS;

    j += (1 + arity) * N_RANDOM_TEST_OPERANDS;

    if (n_words < j)
      {
	test_sets = (n_words == 0
		     ? (word *) xmalloc (sizeof (word) * j)
		     : (word *) xrealloc (test_sets, sizeof (word) * j));
	n_words = j;
      }
  }

  test_set = test_sets;
  j = 0;

  /* Start with all combinations of operands from TEST_OPERANDS.  */
  for (i = arity - 1; i >= 0; i--)
    loop_vars[i] = 0;
  for (;;)
    {
      for (i = arity - 1; i >= 0; i--)
	tvalues[i] = *test_set++ = test_operands[loop_vars[i]];

      /* Get the desired value for the current operand values.  */
      *test_set++ = (*eval)(tvalues);
      j++;

      /* General loop control.  This implements ARITY loops using induction
	 variables in loop_vars[0] through loop_vars[ARITY - 1].  */
      i = 0;
      loop_vars[i] = (loop_vars[i] + 1) % N_TEST_OPERANDS;
      while (loop_vars[i] == 0)
	{
	  i++;
	  if (i >= arity)
	    goto random;
	  loop_vars[i] = (loop_vars[i] + 1) % N_TEST_OPERANDS;
	}
    }

  /* Now add the additional random test sets.  */
 random:
  for (i = N_RANDOM_TEST_OPERANDS - 1; i >= 0; i--)
    {
      for (pc = arity - 1; pc >= 0; pc--)
	tvalues[pc] = *test_set++ = random_word();

      *test_set++ = (*eval)(tvalues);
      j++;
    }

  n_test_sets = j;
}

void
print_operand (int op)
{
  if (IMMEDIATE_P(op))
    {
      if ((signed_word) IMMEDIATE_VAL(op) >= 10
	  || (signed_word) IMMEDIATE_VAL(op) <= -10)
	{
#ifdef PSTR
	  printf(PSTR, IMMEDIATE_VAL(op));
#else
	  word x = IMMEDIATE_VAL(op);
	  if (x >> 32 != 0)
	    {
	      printf("0x%x", (unsigned int) (x >> 32));
	      printf("%08x", (unsigned int) x);
	    }
	  else
	    printf("0x%x", (unsigned int) x);
#endif
	}
      else
	printf("%d", (int) IMMEDIATE_VAL(op));
    }
  else
    printf("r%u", op);
}

/* Test the correctness of a sequence in SEQUENCE[0] through
   SEQUENCE[N_INSNS - 1].  */

void
test_sequence(insn_t *sequence, int n_insns)
{
  int pc;
  int i, j;
  word *test_set = test_sets;
  const int arity = goal_function_arity;

  /* Test each of the precomputed values.  */
  for (j = n_test_sets; j > 0; j--)
    {
      /* Update the tvalues array in each iteration, as execution of the
	 sequence might clobber the values.  (On 2-operand machines.)  */
      for (i = arity - 1; i >= 0; i--)
	tvalues[i] = *test_set++;

      /* Execute the synthesised sequence for the current operand
	 values.  */
#if HAS_NULLIFICATION
      /* Kludge.  run_program returns -2 if a sequence depends on an
	 undefined register.  */
      if (run_program (sequence, n_insns, tvalues, arity) == -2)
	return;
#else
      run_program (sequence, n_insns, tvalues);
#endif
      if (tvalues[sequence[n_insns - 1].d] != *test_set++)
	{
	  /* Adaptively rearrange the order of the tests.  This set of test
	     values is better than all that preceed it.  The optimal
	     ordering changes with the search space.  */
	  if ((j = n_test_sets - j) != 0)
	    {
	      int k = j >> 1;
	      j *= (arity + 1);
	      k *= (arity + 1);
	      for (i = 0; i <= arity; i++)
		{
		  word t = test_sets[j + i];
		  test_sets[j + i] = test_sets[k + i];
		  test_sets[k + i] = t;
		}
	    }
	  return;
	}
    }

#ifdef EXTRA_SEQUENCE_TESTS
  EXTRA_SEQUENCE_TESTS(sequence, n_insns);
#endif

  /* The tests passed.  Print the instruction sequence.  */
  if (success == 0 || flag_nl)
    printf("\n");
  success++;

  printf("%d:", success);
  for (pc = 0; pc < n_insns; pc++)
    {
      insn_t insn;

      insn = sequence[pc];
      if (flag_output_assembly)
	output_assembly(insn);
      else
	{
	  /* Special case for insns that does not affect any general
	     register.  */
	  if (GET_INSN_CLASS (insn.opcode) == '='
	      || GET_INSN_CLASS (insn.opcode) == '<')
	    printf("\t%s(", GET_INSN_NAME(insn.opcode));
	  else
	    printf("\tr%u:=%s(", insn.d, GET_INSN_NAME(insn.opcode));
	  print_operand (insn.s1);

	  if (!UNARY_OPERATION(insn))
	    {
	      printf (",");
	      print_operand (insn.s2);
	    }
	  printf(")\n");
	}
    }
  fflush(stdout);
}

#undef MAKENAME
#define MAKENAME(x)  x
#undef MAKE_LEAF
#define MAKE_LEAF 0

#define RECURSE(opcode, s1, s2, prune_hint) \
  recurse(opcode, n_values, s1, s2, v, 1, sequence, n_insns, values,	\
	  n_values + 1, goal_value, allowed_cost, co, prune_hint)

/* Don't increment n_values unconditionally here, since we often reuse a
   register (if the insn can be nullified) and thus don't generate any more
   values.  Instead, increment n_values when we really use a new destination
   register.  */
#define PA_RECURSE(opcode, d, s1, s2, prune_hint, nullify_flag) \
  recurse(opcode, d, s1, s2, v, 1, sequence, n_insns, values,	\
	  n_values + (n_values == d), goal_value, allowed_cost, co, prune_hint, nullify_flag)

/* CISC recurse, not generating a new register.  */
#define CRECURSE_2OP(opcode, d, s1, s2, cost, prune_hint) \
  recurse(opcode, d, s1, s2, v, cost, sequence, n_insns, values,	\
	  n_values, goal_value, allowed_cost, co, prune_hint)

/* CISC recurse, generating a new register.  */
#define CRECURSE_NEW(opcode, d, s1, s2, cost, prune_hint) \
  recurse(opcode, d, s1, s2, v, cost, sequence, n_insns, values,	\
	  n_values + 1, goal_value, allowed_cost, co, prune_hint)

#include "synth.def"

#undef MAKENAME
#define MAKENAME(x)  x##_last
#undef MAKE_LEAF
#define MAKE_LEAF 1
#undef RECURSE
#define RECURSE(opcode, s1, s2, prune_hint) \
  recurse_last(opcode, n_values, s1, s2, v, sequence, n_insns, goal_value)

/* Don't increment n_values unconditionally here, since we often reuse a
   register (if the insn can be nullified) and thus don't generate any more
   values.  Instead, increment n_values when we really use a new destination
   register.  */
#undef PA_RECURSE
#define PA_RECURSE(opcode, d, s1, s2, prune_hint, nullify_flag) \
  recurse_last(opcode, d, s1, s2, v, sequence, n_insns, goal_value)

/* CISC recurse, not generating a new register.  */
#undef CRECURSE_2OP
#define CRECURSE_2OP(opcode, d, s1, s2, cost, prune_hint) \
  recurse_last(opcode, d, s1, s2, v, sequence, n_insns, goal_value)

/* CISC recurse, generating a new register.  */
#undef CRECURSE_NEW
#define CRECURSE_NEW(opcode, d, s1, s2, cost, prune_hint) \
  recurse_last(opcode, d, s1, s2, v, sequence, n_insns, goal_value)

#if defined (DEBUG)
#include "synth2.def"
#else
#include "synth.def"
#endif


/* Call `synth' allowing deeper and deeper searches for each call.
   This way we achieve iterative deepening search.

   MAXMAX_COST is the maximum cost for any sequence we will accept.  */
void
main_synth(int maxmax_cost, int allowed_extra_cost)
{
  int max_cost;
  word values[0x100];
  insn_t sequence[0x100];
  int i, ii;

  init_immediates(tvalues);
  init_immediates(values);
  init_random_word();
  init_test_sets();

  /* Speed hack: Try to find random values that makes the goal function
     take a value != 0.  Many false sequences give 0 for all input,
     and this hack achieve quick rejection of these sequences.  */
  for (i = 0; i < 50; i++)
    {
      for (ii = 0; ii < goal_function_arity; ii++)
	values[ii] = random_word();
      if ((*eval_goal_function)(values) != 0)
	break;
    }

  ii = 0;
#if KNOW_START
  switch (goal_function)
    {
    case FFS:
#if M88000
      /* Best ffs starting place.  */
      sequence[ii++] = (insn_t) { ADC_CO, CNST(0), 0, 1 };
      sequence[ii++] = (insn_t) { AND, 0, 1, 2 };
#endif
      break;

    case ZDEPI_FOR_MOVSI:
#if SPARC
      sequence[ii++] = (insn_t) { SUB, CNST(0), 0, 1 };
      sequence[ii++] = (insn_t) { AND, 0, 1, 2 };
#endif
      break;
    default: break;
    }
#endif

  printf("Superoptimizing at cost");
  success = 0;
  for (max_cost = 1; max_cost <= maxmax_cost; max_cost++)
    {
#if TIMING
      for (i = 0; i <= max_cost; i++)
	timings[i] = 0;
#endif

      if (success)
	printf ("[cost %d]\n", max_cost + ii);
      else
	printf(" %d", max_cost + ii);
      fflush(stdout);

#if HAS_NULLIFICATION
      i = run_program (sequence, ii, values, goal_function_arity);
#else
      i = run_program (sequence, ii, values);
#endif

      /* Don't pass CY_JUST_SET ever, since we don't know which of the
	 predefined insn above set cy.  */
      SYNTH(sequence, ii, values, goal_function_arity+ii,
	    (*eval_goal_function)(values),
	    max_cost, i, NO_PRUNE
	    , NOT_NULLIFY
	    );

#ifdef STATISTICS
      printf("\n");
      printf("heuristic accept count:%u\n", heuristic_accept_count);
      printf("heuristic reject count:%u\n", heuristic_reject_count);
#endif
#if TIMING
      for (i = 1; i <= max_cost; i++)
	printf ("time %d: %d\n", i, timings[i] - timings[i - 1]);
#endif

      if (success)
	{
	  allowed_extra_cost--;
	  if (allowed_extra_cost < 0)
	    {
	      static char *s[] = {"", "s"};
	      printf("[%d sequence%s found]\n", success,
		      s[success != 1]);
	      return;
	    }
	}
    }
  printf(" failure.\n");
}

/* Create a function for each DEF_GOAL.  When optimized, these are very
   efficient.  */
#undef	DEF_GOAL
#ifdef __STDC__
#define DEF_GOAL(SYM,ARITY,NAME,CODE)	 	\
word SYM ## _func (const word *v)		\
GOAL_FUNCTION (ARITY, CODE)
#else
#define DEF_GOAL(SYM,ARITY,NAME,CODE)	 	\
word SYM/**/_func (v) word *v;			\
GOAL_FUNCTION (ARITY, CODE)
#endif
#define GOAL_FUNCTION(ARITY,CODE)		\
{						\
  word r, v0, v1, v2, v3;			\
  switch (ARITY)				\
    {						\
    default:					\
      abort ();					\
    case 4: v3 = v[3];				\
    case 3: v2 = v[2];				\
    case 2: v1 = v[1];				\
    case 1: v0 = v[0];				\
    }						\
  CODE;						\
  return r;					\
}
#undef	DEF_SYNONYM
#define DEF_SYNONYM(SYM,NAME)
#include "goal.def"

struct
{
  char *fname;
  enum goal_func fcode;
  int arity;
  char *c_code;
  word (*function)(const word*);
} goal_table[] =
{
/* Start off with entries so that goal_names[i].fcode == i.  */
#undef	DEF_GOAL
#ifdef __STDC__
#define DEF_GOAL(SYM,ARITY,NAME,CODE) {NAME, SYM, ARITY, #CODE, SYM ## _func},
#else
#define DEF_GOAL(SYM,ARITY,NAME,CODE) {NAME, SYM, ARITY, "CODE", SYM/**/_func},
#endif
#undef	DEF_SYNONYM
#define DEF_SYNONYM(SYM,NAME)
#include "goal.def"

/* Now add the synonyms.  */
#undef	DEF_GOAL
#define DEF_GOAL(SYM,ARITY,NAME,CODE)
#undef	DEF_SYNONYM
#define DEF_SYNONYM(SYM,NAME) {NAME, SYM, 0, 0},
#include "goal.def"
};

#define GET_GOAL_NAME(X) (goal_table[X].fname)
#define GET_GOAL_ARITY(X) (goal_table[X].arity)
#define GET_GOAL_C_CODE(X) (goal_table[X].c_code)
#define GET_GOAL_FUNCTION(X) (goal_table[X].function)

#ifdef HAVE_RINDEX
#define strrchr rindex
#endif

extern char *strrchr();

int
main(int argc, char **argv)
{
  int maxmax_cost = 4;
  int allowed_extra_cost = 0;
  int flag_all = 0;
  char *program = strrchr (argv[0], '/');

  if (!program)
    program = argv[0];
  else
    program++;

  goal_function = LAST_AND_UNUSED_GOAL_CODE;

  argv++;
  argc--;

  while (argc > 0)
    {
      char *arg = argv[0];
      int arglen = strlen(arg);

      if (arglen < 2)
	arglen = 2;

      if (!strncmp(arg, "-version", arglen))
	{
	  printf ("%s version %s\n", program, version_string);
	  printf ("(%s)\n", TARGET_STRING);
	  if (argc == 1)
	    exit (0);
	}
      else if (!strncmp(arg, "-assembly", arglen))
	flag_output_assembly = 1;
      else if (!strncmp(arg, "-no-carry-insns", arglen))
	flag_use_carry = 0;
      else if (!strncmp(arg, "-all", arglen))
	flag_all = 1;
      else if (!strncmp(arg, "-nl", arglen))
	flag_nl = 1;
      else if (!strncmp(arg, "-shifts", arglen))
	flag_shifts = 1;
      else if (!strncmp(arg, "-extracts", arglen))
	flag_extracts = 1;
      else if (!strncmp(arg, "-max-cost", arglen))
	{
	  argv++;
	  argc--;
	  if (argc == 0)
	    {
	      fprintf(stderr, "superoptimizer: argument to `-max-cost' expected\n");
	      exit(-1);
	    }
	  maxmax_cost = atoi(argv[0]);
	}
      else if (!strncmp(arg, "-extra-cost", arglen))
	{
	  argv++;
	  argc--;
	  if (argc == 0)
	    {
	      fprintf(stderr, "superoptimizer: argument `-extra-cost' expected\n");
	      exit(-1);
	    }
	  allowed_extra_cost = atoi(argv[0]);
	}
      else if (!strncmp(arg, "-f", 2))
	{
	  int i;
	  for (i = 0; i < sizeof(goal_table) / sizeof(goal_table[0]); i++)
	    {
	      if (!strcmp(arg + 2, goal_table[i].fname))
		{
		  goal_function = goal_table[i].fcode;
		  goal_function_arity = GET_GOAL_ARITY(goal_function);
		  eval_goal_function = GET_GOAL_FUNCTION(goal_function);
		  break;
		}
	    }
	  if (goal_function == LAST_AND_UNUSED_GOAL_CODE)
	    {
	      fprintf(stderr, "superoptimizer: unknown goal function\n");
	      exit(-1);
	    }
	}
      else
	{
	  int i, len, maxlen, cols, maxcols;
	  char *prefix;
	  fprintf(stderr, "Calling sequence:\n\n");
	  fprintf(stderr, "\t%s -f<goal-function> [-assembly] [-max-cost n] \\\n", program);
	  fprintf(stderr, "\t\t[-no-carry-insns] [-extra-cost n] [-nl]\n\n");
	  fprintf(stderr, "Target machine: %s\n\n", TARGET_STRING);
	  fprintf(stderr, "Supported goal functions:\n\n");

	  maxlen = 0;
	  for (i = 0; i < sizeof(goal_table) / sizeof(goal_table[0]); i++)
	    {
	      len = strlen (goal_table[i].fname);
	      if (len > maxlen)
		maxlen = len;
	    }

	  maxcols = 79 / (maxlen + 2);
	  if (maxcols < 1)
	    maxcols = 1;

	  cols = 1;
	  prefix = "";
	  for (i = 0; i < sizeof(goal_table) / sizeof(goal_table[0]); i++)
	    {
	      fprintf(stderr, "%s  %-*s", prefix, maxlen, goal_table[i].fname);

	      cols++;
	      if (cols > maxcols)
		{
		  cols = 1;
		  prefix = "\n";
		}
	      else
		prefix = "";
	    }

	  fprintf(stderr, "\n");
	  exit(-1);
	}

      argv++;
      argc--;
    }

  if (flag_all)
    {
      for (goal_function = 0; goal_function < LAST_AND_UNUSED_GOAL_CODE;
	   goal_function++)
	{
	  printf("Searching for goal %s: ",
		  GET_GOAL_NAME (goal_function));
	  printf("%s\n", GET_GOAL_C_CODE(goal_function));

	  goal_function_arity = GET_GOAL_ARITY(goal_function);
	  eval_goal_function = GET_GOAL_FUNCTION(goal_function);
	  main_synth(maxmax_cost, allowed_extra_cost);
	}
      exit (0);
    }

  if (goal_function == LAST_AND_UNUSED_GOAL_CODE)
    {
      fprintf(stderr, "superoptimizer: missing goal function definition\n");
      exit(-1);
    }

  printf("Searching for %s\n", GET_GOAL_C_CODE(goal_function));
  main_synth(maxmax_cost, allowed_extra_cost);
  exit (!success);
}

/* Aux stuff that should go into a separate file.  */

int
ffs_internal(x)
     word x;
{
  int co, ci = -1;
  word d;
  PERFORM_FFS(d, co, x, ci);
  return d;
}

int
floor_log2 (x)
     word x;
{
  register int log = -1;
  while (x != 0)
    log++,
    x >>= 1;
  return log;
}

int
ceil_log2 (x)
     word x;
{
  return floor_log2 (x - 1) + 1;
}

const char clz_tab[] =
{
  32,31,30,30,29,29,29,29,28,28,28,28,28,28,28,28,
  27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,
  26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,
  26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
  24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
};

const char ctz_tab[] =
{
  8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
  4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
};

const char ff1_tab[] =
{
  32,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
};
