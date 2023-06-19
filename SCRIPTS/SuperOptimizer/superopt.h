/* Superoptimizer definitions.

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

#if !(defined(SPARC) || defined(POWER) || defined(POWERPC) || defined(M88000) \
      || defined(AM29K) || defined(MC68000) || defined(MC68020) \
      || defined(I386) || defined(PYR) || defined(ALPHA) || defined(HPPA) \
      || defined(SH) || defined (I960) || defined (I960B))
/* If no target instruction set is defined, use host instruction set.  */
#define SPARC (defined(sparc) || defined(__sparc__))
#define POWER ((defined(rs6000) || defined(_IBMR2)) && !defined (_ARCH_PPC))
#define POWEPC (defined(_ARCH_PPC))
/* #define POWER ???? */
#define M88000 (defined(m88000) || defined(__m88000__))
#define AM29K (defined(_AM29K) || defined(_AM29000))
#define MC68020 (defined(m68020) || defined(mc68020))
#define MC68000 (defined(m68000) || defined(mc68000))
#define I386 (defined(i386) || defined(i80386) || defined(__i386__))
#define PYR (defined(pyr) || defined(__pyr__))
#define ALPHA defined(__alpha)
#define HPPA defined(__hppa)
#define SH defined(__sh__)
#define I960 defined (__i960)
#endif

#define M68000 (MC68000 || MC68020)

#if POWERPC
#define POWER 1
#endif

#if I960B
#define I960 1
#endif

#if SPARC
#define TARGET_STRING "SPARC v7/v8"
#elif POWERPC
#define TARGET_STRING "PowerPC"
#elif POWER
#define TARGET_STRING "IBM POWER"
#elif M88000
#define TARGET_STRING "Motorola MC88000"
#elif AM29K
#define TARGET_STRING "Amd 29000"
#elif MC68020
#define TARGET_STRING "Motorola MC68020"
#elif MC68000
#define TARGET_STRING "Motorola MC68000 (no 68020 instructions)"
#elif I386
#define TARGET_STRING "Intel 386/486/Pentium/Sexium"
#elif PYR
#define TARGET_STRING "Pyramid (with the secret instructions)"
#elif ALPHA
#define TARGET_STRING "DEC Alpha"
#elif HPPA
#define TARGET_STRING "Hewlett-Packard Precision Architecture (PA-RISC)"
#elif  SH
#define TARGET_STRING "Hitachi Super-H (SH)"
/* Reject sequences that require two different registers to be allocated to
   register r0.  */
#define EXTRA_SEQUENCE_TESTS(seq, n) \
{									\
  int reg0 = -1;	/* -1 means r0 is not yet allocated */		\
  int i;								\
  for (i = 0; i < n; i++)						\
    {									\
      if ((seq[i].opcode == AND || seq[i].opcode == XOR			\
	   || seq[i].opcode == IOR || seq[i].opcode == CYEQ)		\
	  && IMMEDIATE_P (seq[i].s2) && IMMEDIATE_VAL (seq[i].s2) != 0)	\
	{								\
	  if (reg0 >= 0 && reg0 != seq[i].d)				\
	    return;							\
	  reg0 = seq[i].d;						\
	}								\
    }									\
}
#elif  I960
#define TARGET_STRING "Intel 960 v1.0"
#elif  I960B
#define TARGET_STRING "Intel 960 v1.1"
#endif

#if !(SPARC || POWER || M88000 || AM29K || M68000 || I386 || PYR \
      || ALPHA || HPPA || SH || I960)
#error You have to choose target CPU type (e.g. -DSPARC).
#endif

#if MC68000
#define SHIFT_COST(CNT) ((8+2*(CNT)) / 5) /* internal cost */
#else
#define SHIFT_COST(CNT) 1
#endif

#if ALPHA
#define BITS_PER_WORD 64
#else
#define BITS_PER_WORD 32
#endif

/* Get longlong.h for double-word arithmetic.
   First define the types for it to operate on.  */
#define UWtype word
#define UHWtype word
#define UDWtype unsigned long	/* Bogus, but we'll not depend on it */
#define W_TYPE_SIZE BITS_PER_WORD
#define SItype int
#define USItype unsigned int
#define DItype long long int
#define UDItype unsigned long long int
#define LONGLONG_STANDALONE
#include "longlong.h"

#if HPPA
#define HAS_NULLIFICATION 1
enum { NOT_NULLIFY = 0, NULLIFY = 1 };
#endif

#if BITS_PER_WORD == 64
#if defined (__GNUC__) || defined (_LONGLONG)
typedef unsigned long long int unsigned_word;
typedef signed long long int signed_word;
typedef unsigned_word word;
#undef PSTR			/* no portable way to print */
#elif __alpha /* Native compiler on alpha has 64 bit longs.  */
typedef unsigned long int unsigned_word;
typedef signed long int signed_word;
typedef unsigned_word word;
#define PSTR "0x%lx"
#else /* Not on alpha, not GCC.  Don't have 64 bit type.  */
#error Do not know how to perform 64 bit arithmetic with this compiler.
#endif
#else
typedef unsigned int unsigned_word;
typedef signed int signed_word;
typedef unsigned_word word;
#define PSTR "0x%x"
#endif


#define TRUNC_CNT(cnt) ((unsigned) (cnt) % BITS_PER_WORD)

#if defined(sparc) || defined(__GNUC__)
#define alloca __builtin_alloca
#endif

#if !defined(__GNUC__) || !defined(__OPTIMIZE__) || defined(DEBUG)
#define inline /* Empty */
#endif

/* Handle immediates by putting all handled values in the VALUE array at
   appropriate indices, and then insert these indices in the code.
   We currently do this just for some hardwired constants.  */

#define CNST_0x80000000 (0x20 - 2)
#define CNST_0x7FFFFFFF (0x20 - 3)
#define CNST_0xFFFF (0x20 - 4)
#define CNST_0xFF (0x20 - 5)

#define VALUE_MIN_SIGNED ((word) 1 << (BITS_PER_WORD - 1))
#define VALUE_MAX_SIGNED (((word) 1 << (BITS_PER_WORD - 1)) - 1)

#define CNST(n) (0x20 + n)
#define VALUE(n) n

/* The IMMEDIATE_* macros are for printing assembly.  NOT for sequence
   generating or analyze.  */
#define IMMEDIATE_P(op) (op >= 0x20 - 5)
static const word __immediate_val[] =
{
  VALUE_MIN_SIGNED,
  VALUE_MAX_SIGNED,
  0xffff,
  0xff
};
#define IMMEDIATE_VAL(op) \
  ((op) >= 0x20 - 1 ? op - 0x20 : __immediate_val[0x20 - 2 - (op)])

typedef enum
{
#undef DEF_INSN
#define DEF_INSN(SYM,CLASS,NAME) SYM,
#include "insn.def"
} opcode_t;

#define GET_INSN_CLASS(OP) (insn_class[OP])
#define GET_INSN_NAME(OP) (insn_name[OP])

#define UNARY_OPERATION(insn) (GET_INSN_CLASS (insn.opcode) == '1')

typedef struct
{
  opcode_t opcode:11;
  unsigned int s1:7;
  unsigned int s2:7;
  unsigned int d:7;
} insn_t;

#if __GNUC__ < 2
#define __CLOBBER_CC
#define __AND_CLOBBER_CC
#else /* __GNUC__ >= 2 */
#define __CLOBBER_CC : "cc"
#define __AND_CLOBBER_CC , "cc"
#endif /* __GNUC__ < 2 */

/* PERFORM_* for all instructions the search uses.  These macros are
   used both in the search phase and in the test phase.  */

#if defined(__GNUC__) && defined(USE_ASM)
/*** Define machine-dependent PERFORM_* here to improve synthesis speed ***/

#if sparc
#define PERFORM_ADD_CIO(d, co, r1, r2, ci) \
  asm ("subcc %%g0,%4,%%g0	! set cy if CI != 0
	addxcc %2,%3,%0		! add R1 and R2
	addx %%g0,%%g0,%1	! set CO to cy"				\
       : "=r" (d), "=r" (co)						\
       : "%r" (r1), "rI" (r2), "rI" (ci)				\
       __CLOBBER_CC)
#define PERFORM_ADD_CO(d, co, r1, r2, ci) \
  asm ("addcc %2,%3,%0		! add R1 and R2
	addx %%g0,%%g0,%1	! set CO to cy"				\
       : "=r" (d), "=r" (co)						\
       : "%r" (r1), "rI" (r2)						\
       __CLOBBER_CC)
#define PERFORM_SUB_CIO(d, co, r1, r2, ci) \
  asm ("subcc %%g0,%4,%%g0	! set cy if CI != 0
	subxcc %2,%3,%0		! subtract R2 from R1
	addx %%g0,%%g0,%1	! set CO to cy"				\
       : "=r" (d), "=r" (co)						\
       : "r" (r1), "rI" (r2), "rI" (ci)					\
       __CLOBBER_CC)
#define PERFORM_SUB_CO(d, co, r1, r2, ci) \
  asm ("subcc %2,%3,%0		! subtract R2 from R1
	addx %%g0,%%g0,%1	! set CO to cy"				\
       : "=r" (d), "=r" (co)						\
       : "r" (r1), "rI" (r2)						\
       __CLOBBER_CC)
#define PERFORM_ADC_CIO(d, co, r1, r2, ci) \
  asm ("subcc %4,1,%%g0		! cy = (CI == 0)
	subxcc %2,%3,%0		! subtract R2 from R1
	subx %%g0,-1,%1		! set CO to !cy"			\
       : "=&r" (d), "=r" (co)						\
       : "r" (r1), "rI" (r2), "rI" (ci)					\
       __CLOBBER_CC)
#define PERFORM_ADC_CO(d, co, r1, r2, ci) \
  asm ("subcc %2,%3,%0		! subtract R2 from R1
	subx %%g0,-1,%1		! set CO to !cy"			\
       : "=&r" (d), "=r" (co)						\
       : "r" (r1), "rI" (r2)						\
       __CLOBBER_CC)
#endif /* sparc */

#if m88k
#define PERFORM_ADD_CIO(d, co, r1, r2, ci) \
  asm ("or %0,r0,1
	subu.co r0,%4,%0	; set cy if CI != 0
	addu.cio %0,%2,%r3	; add R1 and R2
	addu.ci %1,r0,r0	; set CO to cy"				\
       : "=&r" (d), "=r" (co)						\
       : "%r" (r1), "Or" (r2), "r" (ci))
#define PERFORM_ADD_CO(d, co, r1, r2, ci) \
  asm ("addu.co %0,%2,%r3	; add R1 and R2
	addu.ci %1,r0,r0	; set CO to cy"				\
       : "=r" (d), "=r" (co)						\
       : "%r" (r1), "Or" (r2))
#define PERFORM_SUB_CIO(d, co, r1, r2, ci) \
  asm ("subu.co r0,r0,%r4	; reset cy if CI != 0
	subu.cio %0,%2,%r3	; subtract R2 from R1
	subu.ci %1,r0,r0	; set CO to -1+cy
	subu %1,r0,%1		; set CO to !cy"			\
       : "=r" (d), "=r" (co)						\
       : "r" (r1), "Or" (r2), "Or" (ci))
#define PERFORM_SUB_CO(d, co, r1, r2, ci) \
  asm ("subu.co %0,%2,%r3	; subtract R2 from R1
	subu.ci %1,r0,r0	; set CO to -1+cy
	subu %1,r0,%1		; set CO to !cy"			\
       : "=r" (d), "=r" (co)						\
       : "r" (r1), "Or" (r2))
#define PERFORM_ADC_CIO(d, co, r1, r2, ci) \
  asm ("or %0,r0,1
	subu.co r0,%r4,%0	; set cy if CI != 0
	subu.cio %0,%2,%r3	; subtract R2 from R1
	addu.ci %1,r0,r0	; set CO to cy"				\
       : "=&r" (d), "=r" (co)						\
       : "r" (r1), "Or" (r2), "Or" (ci))
#define PERFORM_ADC_CO(d, co, r1, r2, ci) \
  asm ("subu.co %0,%2,%r3	; subtract R2 from R1
	addu.ci %1,r0,r0	; set CO to cy"				\
       : "=r" (d), "=r" (co)						\
       : "r" (r1), "Or" (r2))
#endif /* m88k */

#endif /* __GNUC__ && USE_ASM */

/************************* Default PERFORM_* in C *************************/

#define PERFORM_COPY(d, co, r1, ci) \
  ((d) = (r1), (co) = (ci))
#define PERFORM_EXCHANGE(co, r1, r2, ci) \
  do {word __temp = (r1), (r1) = (r2), (r2) = __temp, (co) = (ci);} while (0)

#define PERFORM_ADD(d, co, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci))
#ifndef PERFORM_ADD_CIO
#define PERFORM_ADD_CIO(d, co, r1, r2, ci) \
  do { word __d = (r1) + (ci);						\
       word __cy = __d < (ci);						\
       (d) = __d + (r2);						\
       (co) = ((d) < __d) + __cy; } while (0)
#endif
#ifndef PERFORM_ADD_CI
#define PERFORM_ADD_CI(d, co, r1, r2, ci) \
  do { word __d = (r1) + (r2) + (ci);					\
       (co) = (ci);							\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_ADD_CO
#define PERFORM_ADD_CO(d, co, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       (co) = __d < (r1);						\
       (d) = __d; } while (0)
#endif

#define PERFORM_SUB(d, co, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci))
#ifndef PERFORM_SUB_CIO
#define PERFORM_SUB_CIO(d, co, r1, r2, ci) \
  do { word __d = (r1) - (r2) - (ci);					\
       (co) = (ci) ? __d >= (r1) : __d > (r1);				\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_SUB_CI
#define PERFORM_SUB_CI(d, co, r1, r2, ci) \
  do { word __d = (r1) - (r2) - (ci);					\
       (co) = (ci);							\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_SUB_CO
#define PERFORM_SUB_CO(d, co, r1, r2, ci) \
  do { word __d = (r1) - (r2);						\
       (co) = __d > (r1);						\
       (d) = __d; } while (0)
#endif

#ifndef PERFORM_ADC_CIO
#define PERFORM_ADC_CIO(d, co, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + (ci);					\
       (co) = (ci) ? __d <= (r1) : __d < (r1);				\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_ADC_CI
#define PERFORM_ADC_CI(d, co, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + (ci);					\
       (co) = (ci);							\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_ADC_CO
#define PERFORM_ADC_CO(d, co, r1, r2, ci) \
  do { word __d = (r1) - (r2);						\
       (co) = __d <= (r1);						\
       (d) = __d; } while (0)
#endif

#ifndef PERFORM_ADDCMPL
#define PERFORM_ADDCMPL(d, co, r1, r2, ci) \
  ((d) = (r1) + ~(r2), (co) = (ci))
#endif

#ifndef PERFORM_CMP
#define PERFORM_CMP(d, co, r1, r2, ci) \
  ((co) = (r1) < (r2))
#endif
#ifndef PERFORM_CMPPAR
#define PERFORM_CMPPAR(d, co, r1, r2, ci) \
  do {									\
    word __x;								\
    union { long w; short h[2]; char b[4]; } __r1, __r2;		\
    __r1.w = (r1); __r2.w = (r2);					\
    __x = ((__r1.h[0] != __r2.h[0]) && (__r1.h[1] != __r2.h[1])) << 14;	\
    __x |= ((__r1.b[0] != __r2.b[0]) && (__r1.b[1] != __r2.b[1])	\
	   && (__r1.b[2] != __r2.b[2]) && (__r1.b[3] != __r2.b[3])) << 12; \
    __x |= ((unsigned_word) (r1) >= (unsigned_word) (r2)) << 10;	\
    __x |= ((unsigned_word) (r1) <= (unsigned_word) (r2)) << 8;		\
    __x |= ((signed_word) (r1) >= (signed_word) (r2)) << 6;		\
    __x |= ((signed_word) (r1) <= (signed_word) (r2)) << 4;		\
    __x |= ((r1) != (r2)) << 2;						\
    (d) = __x + 0x5554;		/* binary 0101010101010100 */		\
    (co) = (ci);							\
  } while (0)
#endif

/* Logic operations that don't affect carry.  */
#ifndef PERFORM_AND
#define PERFORM_AND(d, co, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci))
#endif
#ifndef PERFORM_IOR
#define PERFORM_IOR(d, co, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci))
#endif
#ifndef PERFORM_XOR
#define PERFORM_XOR(d, co, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci))
#endif
#ifndef PERFORM_ANDC
#define PERFORM_ANDC(d, co, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci))
#endif
#ifndef PERFORM_IORC
#define PERFORM_IORC(d, co, r1, r2, ci) \
  ((d) = (r1) | ~(r2), (co) = (ci))
#endif
#ifndef PERFORM_EQV
#define PERFORM_EQV(d, co, r1, r2, ci) \
  ((d) = (r1) ^ ~(r2), (co) = (ci))
#endif
#ifndef PERFORM_NAND
#define PERFORM_NAND(d, co, r1, r2, ci) \
  ((d) = ~((r1) & (r2)), (co) = (ci))
#endif
#ifndef PERFORM_NOR
#define PERFORM_NOR(d, co, r1, r2, ci) \
  ((d) = ~((r1) | (r2)), (co) = (ci))
#endif

/* Logic operations that reset carry.  */
#ifndef PERFORM_AND_RC
#define PERFORM_AND_RC(d, co, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = 0)
#endif
#ifndef PERFORM_IOR_RC
#define PERFORM_IOR_RC(d, co, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = 0)
#endif
#ifndef PERFORM_XOR_RC
#define PERFORM_XOR_RC(d, co, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = 0)
#endif
#ifndef PERFORM_ANDC_RC
#define PERFORM_ANDC_RC(d, co, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = 0)
#endif
#ifndef PERFORM_IORC_RC
#define PERFORM_IORC_RC(d, co, r1, r2, ci) \
  ((d) = (r1) | ~(r2), (co) = 0)
#endif
#ifndef PERFORM_EQV_RC
#define PERFORM_EQV_RC(d, co, r1, r2, ci) \
  ((d) = (r1) ^ ~(r2), (co) = 0)
#endif
#ifndef PERFORM_NAND_RC
#define PERFORM_NAND_RC(d, co, r1, r2, ci) \
  ((d) = ~((r1) & (r2)), (co) = 0)
#endif
#ifndef PERFORM_NOR_RC
#define PERFORM_NOR_RC(d, co, r1, r2, ci) \
  ((d) = ~((r1) | (r2)), (co) = 0)
#endif

/* Logic operations that clobber carry.  */
#ifndef PERFORM_AND_CC
#define PERFORM_AND_CC(d, co, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = -1)
#endif
#ifndef PERFORM_IOR_CC
#define PERFORM_IOR_CC(d, co, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = -1)
#endif
#ifndef PERFORM_XOR_CC
#define PERFORM_XOR_CC(d, co, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = -1)
#endif
#ifndef PERFORM_ANDC_CC
#define PERFORM_ANDC_CC(d, co, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = -1)
#endif
#ifndef PERFORM_IORC_CC
#define PERFORM_IORC_CC(d, co, r1, r2, ci) \
  ((d) = (r1) | ~(r2), (co) = -1)
#endif
#ifndef PERFORM_EQV_CC
#define PERFORM_EQV_CC(d, co, r1, r2, ci) \
  ((d) = (r1) ^ ~(r2), (co) = -1)
#endif
#ifndef PERFORM_NAND_CC
#define PERFORM_NAND_CC(d, co, r1, r2, ci) \
  ((d) = ~((r1) & (r2)), (co) = -1)
#endif
#ifndef PERFORM_NOR_CC
#define PERFORM_NOR_CC(d, co, r1, r2, ci) \
  ((d) = ~((r1) | (r2)), (co) = -1)
#endif

#ifndef PERFORM_LSHIFTR
#define PERFORM_LSHIFTR(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >> TRUNC_CNT(r2)),			\
   (co) = (ci))
#endif
#ifndef PERFORM_ASHIFTR
#define PERFORM_ASHIFTR(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)),				\
   (co) = (ci))
#endif
#ifndef PERFORM_SHIFTL
#define PERFORM_SHIFTL(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) << TRUNC_CNT(r2)), (co) = (ci))
#endif
#ifndef PERFORM_ROTATEL
#define PERFORM_ROTATEL(d, co, r1, r2, ci) \
 ((d) = TRUNC_CNT(r2) == 0 ? (r1)					\
  : ((r1) << TRUNC_CNT(r2)) | ((r1) >> TRUNC_CNT(BITS_PER_WORD - (r2))),\
  (co) = (ci))
#endif
#ifndef PERFORM_LSHIFTR_CO
#define PERFORM_LSHIFTR_CO(d, co, r1, r2, ci) \
  do { word __d = ((unsigned_word) (r1) >> TRUNC_CNT(r2));		\
       (co) = ((unsigned_word) (r1) >> (TRUNC_CNT(r2) - 1)) & 1;	\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_ASHIFTR_CO
#define PERFORM_ASHIFTR_CO(d, co, r1, r2, ci) \
  do { word __d = ((signed_word) (r1) >> TRUNC_CNT(r2));		\
       (co) = ((signed_word) (r1) >> (TRUNC_CNT(r2) - 1)) & 1;		\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_ASHIFTR_CON
#define PERFORM_ASHIFTR_CON(d, co, r1, r2, ci) \
  do { word __d = ((signed_word) (r1) >> TRUNC_CNT(r2));		\
	 (co) = (signed_word) (r1) < 0					\
	   && ((r1) << TRUNC_CNT(BITS_PER_WORD - (r2))) != 0;		\
       (d) = __d; } while (0)
#endif
#ifndef PERFORM_SHIFTL_CO
#define PERFORM_SHIFTL_CO(d, co, r1, r2, ci) \
  do { word __d = ((unsigned_word) (r1) << TRUNC_CNT(r2));		\
       (co) = ((r1) >> TRUNC_CNT(BITS_PER_WORD - (r2))) & 1;		\
       (d) = __d; } while (0)
#endif
/* Do these first two rotates actually set carry correctly for r2 == 0??? */
#ifndef PERFORM_ROTATEL_CO
#define PERFORM_ROTATEL_CO(d, co, r1, r2, ci) \
  ((d) = (((r1) << TRUNC_CNT(r2))					\
	  | ((unsigned_word) (r1) >> TRUNC_CNT(BITS_PER_WORD - (r2)))),	\
   (co) = (d) & 1)
#endif
#ifndef PERFORM_ROTATER_CO
#define PERFORM_ROTATER_CO(d, co, r1, r2, ci) \
  ((d) = (((r1) >> TRUNC_CNT(r2))					\
	  | ((unsigned_word) (r1) << TRUNC_CNT(BITS_PER_WORD - (r2)))),	\
   (co) = ((d) >> (BITS_PER_WORD - 1)) & 1)
#endif
#ifndef PERFORM_ROTATEXL_CIO
#define PERFORM_ROTATEXL_CIO(d, co, r1, r2, ci) \
  do { word __d;  unsigned cnt = TRUNC_CNT(r2);				\
       if (cnt != 0)							\
	 {								\
	   __d = ((r1) << cnt) | ((ci) << (cnt - 1));			\
	   if (cnt != 1)						\
	     __d |= (unsigned_word) (r1) >> (BITS_PER_WORD - (cnt - 1));\
	   (co) = ((unsigned_word) (r1) >> (BITS_PER_WORD - cnt)) & 1;	\
	   (d) = __d;							\
	 }								\
       else								\
	 {								\
	   (co) = (ci);							\
	   (d) = (r1);							\
	 }								\
     } while (0)
#endif
#ifndef PERFORM_ROTATEXR_CIO
#define PERFORM_ROTATEXR_CIO(d, co, r1, r2, ci) \
  do { word __d;  unsigned cnt = TRUNC_CNT(r2);				\
       if (cnt != 0)							\
	 {								\
	   __d = ((unsigned_word) (r1) >> cnt) | ((ci) << (BITS_PER_WORD - cnt)); \
	   if (cnt != 1)						\
	     __d |= ((r1) << (BITS_PER_WORD - (cnt - 1)));		\
	   (co) = ((unsigned_word) (r1) >> (cnt - 1)) & 1;		\
	   (d) = __d;							\
	 }								\
       else								\
	 {								\
	   (co) = (ci);							\
	   (d) = (r1);							\
	 }								\
     } while (0)
#endif
#ifndef PERFORM_EXTS1
#define PERFORM_EXTS1(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 31 >> 31, (co) = (ci))
#endif
#ifndef PERFORM_EXTS2
#define PERFORM_EXTS2(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 30 >> 30, (co) = (ci))
#endif
#ifndef PERFORM_EXTS8
#define PERFORM_EXTS8(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 24 >> 24, (co) = (ci))
#endif
#ifndef PERFORM_EXTS16
#define PERFORM_EXTS16(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 16 >> 16, (co) = (ci))
#endif
#ifndef PERFORM_EXTU1
#define PERFORM_EXTU1(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >> TRUNC_CNT(r2)) & 1, (co) = (ci))
#endif
#ifndef PERFORM_EXTU2
#define PERFORM_EXTU2(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >> TRUNC_CNT(r2)) & 3, (co) = (ci))
#endif

#ifndef PERFORM_DOZ
#define PERFORM_DOZ(d, co, r1, r2, ci) \
  (((d) = (signed_word) (r1) > (signed_word) (r2) ? (r1) - (r2) : 0),	\
   (co) = (ci))
#endif

#ifndef PERFORM_CPEQ
#define PERFORM_CPEQ(d, co, r1, r2, ci) \
  ((d) = ((r1) == (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPGE
#define PERFORM_CPGE(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >= (signed_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPGEU
#define PERFORM_CPGEU(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >= (unsigned_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPGT
#define PERFORM_CPGT(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) > (signed_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPGTU
#define PERFORM_CPGTU(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) > (unsigned_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPLE
#define PERFORM_CPLE(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) <= (signed_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPLEU
#define PERFORM_CPLEU(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) <= (unsigned_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPLT
#define PERFORM_CPLT(d, co, r1, r2, ci) \
  ((d) = ((signed_word) (r1) < (signed_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPLTU
#define PERFORM_CPLTU(d, co, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) < (unsigned_word) (r2)) << 31, (co) = (ci))
#endif
#ifndef PERFORM_CPNEQ
#define PERFORM_CPNEQ(d, co, r1, r2, ci) \
  ((d) = ((r1) != (r2)) << 31, (co) = (ci))
#endif

#ifndef PERFORM_CMPEQ
#define PERFORM_CMPEQ(d, co, r1, r2, ci) \
  ((d) = (r1) == (r2), (co) = (ci))
#endif
#ifndef PERFORM_CMPLE
#define PERFORM_CMPLE(d, co, r1, r2, ci) \
  ((d) = (signed_word) (r1) <= (signed_word) (r2), (co) = (ci))
#endif
#ifndef PERFORM_CMPLEU
#define PERFORM_CMPLEU(d, co, r1, r2, ci) \
  ((d) = (unsigned_word) (r1) <= (unsigned_word) (r2), (co) = (ci))
#endif
#ifndef PERFORM_CMPLT
#define PERFORM_CMPLT(d, co, r1, r2, ci) \
  ((d) = (signed_word) (r1) < (signed_word) (r2), (co) = (ci))
#endif
#ifndef PERFORM_CMPLTU
#define PERFORM_CMPLTU(d, co, r1, r2, ci) \
  ((d) = (unsigned_word) (r1) < (unsigned_word) (r2), (co) = (ci))
#endif

#ifndef PERFORM_CYEQ
#define PERFORM_CYEQ(d, co, r1, r2, ci) \
  ((co) = (r1) == (r2))
#endif
#ifndef PERFORM_CYGES
#define PERFORM_CYGES(d, co, r1, r2, ci) \
  ((co) = (signed_word) (r1) >= (signed_word) (r2))
#endif
#ifndef PERFORM_CYGEU
#define PERFORM_CYGEU(d, co, r1, r2, ci) \
  ((co) = (unsigned_word) (r1) >= (unsigned_word) (r2))
#endif
#ifndef PERFORM_CYGTS
#define PERFORM_CYGTS(d, co, r1, r2, ci) \
  ((co) = (signed_word) (r1) > (signed_word) (r2))
#endif
#ifndef PERFORM_CYGTU
#define PERFORM_CYGTU(d, co, r1, r2, ci) \
  ((co) = (unsigned_word) (r1) > (unsigned_word) (r2))
#endif
#ifndef PERFORM_CYAND
#define PERFORM_CYAND(d, co, r1, r2, ci) \
  ((co) = ((r1) & (r2)) == 0)
#endif
#ifndef PERFORM_MERGE16
#define PERFORM_MERGE16(d, co, r1, r2, ci) \
  ((d) = ((word) (r1) >> 16) | ((r2) << 16), (co) = (ci))
#endif
#ifndef PERFORM_DECR_CYEQ
#define PERFORM_DECR_CYEQ(d, co, r1, r2, ci) \
  ((d) = ((r1) - 1), (co) = (r1) == (r2))   /* should protect from samevar(d,r1/r2) ??? */
#endif

/* Unary operations.  */
#ifndef PERFORM_CLZ
#define PERFORM_CLZ(d, co, r1, ci) \
  do {									\
    int __a = 0;							\
    word __r = (r1);							\
    if (__r > 0xffffffff)						\
      __r = __r >> 31 >> 1, __a += 32;					\
    if (__r > 0xffff)							\
      __r >>= 16, __a += 16;						\
    if (__r > 0xff)							\
      __r >>= 8, __a += 8;						\
    (d) = clz_tab[__r] - __a + BITS_PER_WORD - 32;			\
    (co) = (ci);							\
  } while (0)
#endif
#ifndef PERFORM_CTZ
/* This can be done faster using the (x & -x) trick.  */
#define PERFORM_CTZ(d, co, r1, ci) \
  do {									\
    int __a;								\
    abort ();								\
    __a = ((r1) & 0xffff == 0)						\
      ? (((r1) & 0xff0000) == 0 ? 24 : 16)				\
      : ((r1) & 0xff == 0) ? 8 : 0;					\
    (d) = ctz_tab[((r1) >> __a) & 0xff] + __a;				\
    (co) = (ci);							\
  } while (0)
#endif
#ifndef PERFORM_FF1
#define PERFORM_FF1(d, co, r1, ci) \
  do {									\
    int __a;								\
    __a = (r1) <= 0xffff						\
      ? ((r1) <= 0xff ? 0 : 8)						\
      : ((r1) <= 0xffffff ?  16 : 24);					\
    (d) = ff1_tab[(r1) >> __a] + __a;					\
    (co) = (ci);							\
  } while (0)
#endif
#ifndef PERFORM_FF0
#define PERFORM_FF0(d, co, r1, ci) \
  PERFORM_FF1(d, co, ~(r1), ci)
#endif
#ifndef PERFORM_FFS
#define PERFORM_FFS(d, co, r1, ci) \
  do {									\
    word __x = (r1) & (-r1);						\
    PERFORM_CLZ(d, co, __x, ci);					\
    (d) = BITS_PER_WORD - (d);						\
  } while (0)
#endif
#ifndef PERFORM_BSF86
#define PERFORM_BSF86(d, co, r1, ci) \
  do {                                                                  \
    if ((r1) == 0)							\
      (d) = random_word ();						\
    else								\
      PERFORM_FF1(d, co, (r1) & -(r1), ci);				\
    (co) = -1;								\
  } while (0)
#endif
#ifndef PERFORM_ABSVAL
#define PERFORM_ABSVAL(d, co, r1, ci) \
  ((d) = (signed_word) (r1) < 0 ? -(r1) : (r1), (co) = (ci))
#endif
#ifndef PERFORM_NABSVAL
#define PERFORM_NABSVAL(d, co, r1, ci) \
  ((d) = (signed_word) (r1) > 0 ? -(r1) : (r1), (co) = (ci))
#endif

#ifndef PERFORM_CMOVEQ
#define PERFORM_CMOVEQ(d, co, r1, r2, ci) \
  ((d) = (r1) == 0 ? (r2) : (d), (co) = (ci))
#endif
#ifndef PERFORM_CMOVNE
#define PERFORM_CMOVNE(d, co, r1, r2, ci) \
  ((d) = (r1) != 0 ? (r2) : (d), (co) = (ci))
#endif
#ifndef PERFORM_CMOVLT
#define PERFORM_CMOVLT(d, co, r1, r2, ci) \
  ((d) = (signed_word) (r1) < 0 ? (r2) : (d), (co) = (ci))
#endif
#ifndef PERFORM_CMOVGE
#define PERFORM_CMOVGE(d, co, r1, r2, ci) \
  ((d) = (signed_word) (r1) >= 0 ? (r2) : (d), (co) = (ci))
#endif
#ifndef PERFORM_CMOVLE
#define PERFORM_CMOVLE(d, co, r1, r2, ci) \
  ((d) = (signed_word) (r1) <= 0 ? (r2) : (d), (co) = (ci))
#endif
#ifndef PERFORM_CMOVGT
#define PERFORM_CMOVGT(d, co, r1, r2, ci) \
  ((d) = (signed_word) (r1) > 0 ? (r2) : (d), (co) = (ci))
#endif

#ifndef PERFORM_INVDIV
#define PERFORM_INVDIV(v, co, r1, ci) \
  do {									\
    word __q, __r;							\
    udiv_qrnnd (__q, __r, -(r1), 0, (r1));				\
    (v) = __q;								\
    (co) = (ci);							\
  } while (0)
#endif
#ifndef PERFORM_INVMOD
#define PERFORM_INVMOD(v, co, r1, ci) \
  do {									\
    word __q, __r;							\
    udiv_qrnnd (__q, __r, -(r1), 0, (r1));				\
    (v) = __r;								\
    (co) = (ci);							\
  } while (0)
#endif
#ifndef PERFORM_MUL
#define PERFORM_MUL(v, co, r1, r2, ci) \
  do {									\
    (v) = (r1) * (r2);							\
    (co) = (ci);							\
  } while (0)
#endif
#ifndef PERFORM_UMULWIDEN_HI
#define PERFORM_UMULWIDEN_HI(v, co, r1, r2, ci) \
  do {									\
    word __ph, __pl;							\
    umul_ppmm (__ph, __pl, (r1), (r2));					\
    (v) = __ph;								\
    (co) = (ci);							\
  } while (0)
#endif

#ifdef UDIV_WITH_SDIV
#define PERFORM_SDIV(v, co, r1, r2, ci) \
  do {									\
    if ((r2) != 0)							\
      (v) = (signed_word) (r1) / (signed_word) (r2);			\
    else								\
      (v) = 0;								\
    (co) = (ci);							\
    } while (0)
#endif /* UDIV_WITH_SDIV */

/* HP-PA nullifying instructions.  */
#define PERFORM_NULLIFIED(v, co, sc, ci) \
  ((v) = values[dr], (co) = (ci), (sc) = 0)
#define PERFORM_COPY_S(d, co, sc, r1, ci) \
  ((d) = (r1), (co) = (ci), (sc) = 1)

#define PERFORM_ADD_SEQ(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (r1) == -(r2))
#define PERFORM_ADD_SNE(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (r1) != -(r2))
#define PERFORM_ADD_SLTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (signed_word) (r1) < -(signed_word) (r2))
#define PERFORM_ADD_SGES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (signed_word) (r1) >= -(signed_word) (r2))
#define PERFORM_ADD_SLES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (signed_word) (r1) <= -(signed_word) (r2))
#define PERFORM_ADD_SGTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (signed_word) (r1) > -(signed_word) (r2))
#define PERFORM_ADD_SLTU(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (unsigned_word) (r1) < -(unsigned_word) (r2))
#define PERFORM_ADD_SGEU(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (unsigned_word) (r1) >= -(unsigned_word) (r2))
#define PERFORM_ADD_SLEU(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (unsigned_word) (r1) <= -(unsigned_word) (r2))
#define PERFORM_ADD_SGTU(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (unsigned_word) (r1) > -(unsigned_word) (r2))
#define PERFORM_ADD_SOVS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (signed_word) (~((r1) ^ (r2)) & ((d) ^ (r1))) < 0)
#define PERFORM_ADD_SNVS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = (signed_word) (~((r1) ^ (r2)) & ((d) ^ (r1))) >= 0)
#define PERFORM_ADD_SODD(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = ((d) & 1) != 0)
#define PERFORM_ADD_SEVN(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci),					\
   sc = ((d) & 1) == 0)
#define PERFORM_ADD_S(d, co, sc, r1, r2, ci) \
  ((d) = (r1) + (r2), (co) = (ci), (sc) = 1)

#define PERFORM_ADD_CIO_SEQ(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d == 0; } while (0)
#define PERFORM_ADD_CIO_SNE(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d != 0; } while (0)
#define PERFORM_ADD_CIO_SLTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0; } while (0)
#define PERFORM_ADD_CIO_SGEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy; } while (0)
#define PERFORM_ADD_CIO_SLEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0 || __d == 0; } while (0)
#define PERFORM_ADD_CIO_SGTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy && __d != 0; } while (0)
#define PERFORM_ADD_CIO_SODD(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) != 0; } while (0)
#define PERFORM_ADD_CIO_SEVN(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) == 0; } while (0)
#define PERFORM_ADD_CIO_S(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = 1; } while (0)

#define PERFORM_ADD_CO_SEQ(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d == 0; } while (0)
#define PERFORM_ADD_CO_SNE(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d != 0; } while (0)
#define PERFORM_ADD_CO_SLTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0; } while (0)
#define PERFORM_ADD_CO_SGEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy; } while (0)
#define PERFORM_ADD_CO_SLEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0 || __d == 0; } while (0)
#define PERFORM_ADD_CO_SGTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy && __d != 0; } while (0)
#define PERFORM_ADD_CO_SODD(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) != 0; } while (0)
#define PERFORM_ADD_CO_SEVN(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) == 0; } while (0)
#define PERFORM_ADD_CO_S(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + (r2);						\
       word __cy = __d < (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = 1; } while (0)

/* These are used with the hppa andc instruction to complement stuff.
   The conditions might be incorrect for any other usage.  */
#define PERFORM_SUB_SEQ(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = (d) == 0)
#define PERFORM_SUB_SNE(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = (d) != 0)
#define PERFORM_SUB_SLTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = (signed_word) (d) < 0)
#define PERFORM_SUB_SGES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = (signed_word) (d) >= 0)
#define PERFORM_SUB_SLES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = (signed_word) (d) <= 0)
#define PERFORM_SUB_SGTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = (signed_word) (d) > 0)
#define PERFORM_SUB_SODD(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = ((d) & 1) != 0)
#define PERFORM_SUB_SEVN(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci),					\
   sc = ((d) & 1) == 0)
#define PERFORM_SUB_S(d, co, sc, r1, r2, ci) \
  ((d) = (r1) - (r2), (co) = (ci), (sc) = 1)

#define PERFORM_ADC_CIO_SEQ(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d == 0; } while (0)
#define PERFORM_ADC_CIO_SNE(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d != 0; } while (0)
#define PERFORM_ADC_CIO_SLTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0; } while (0)
#define PERFORM_ADC_CIO_SGEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy; } while (0)
#define PERFORM_ADC_CIO_SLEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0 || __d == 0; } while (0)
#define PERFORM_ADC_CIO_SGTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy && __d != 0; } while (0)
#define PERFORM_ADC_CIO_SODD(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) != 0; } while (0)
#define PERFORM_ADC_CIO_SEVN(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) == 0; } while (0)
#define PERFORM_ADC_CIO_S(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2);						\
       word __cy = __d < (r1);						\
       __d += (ci);							\
       __cy += (__d < (ci));						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = 1; } while (0)

#define PERFORM_ADC_CO_SEQ(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d == 0; } while (0)
#define PERFORM_ADC_CO_SNE(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __d != 0; } while (0)
#define PERFORM_ADC_CO_SLTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0; } while (0)
#define PERFORM_ADC_CO_SGEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy; } while (0)
#define PERFORM_ADC_CO_SLEU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy == 0 || __d == 0; } while (0)
#define PERFORM_ADC_CO_SGTU(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = __cy && __d != 0; } while (0)
#define PERFORM_ADC_CO_SODD(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) != 0; } while (0)
#define PERFORM_ADC_CO_SEVN(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = (__d & 1) == 0; } while (0)
#define PERFORM_ADC_CO_S(d, co, sc, r1, r2, ci) \
  do { word __d = (r1) + ~(r2) + 1;					\
       word __cy = __d <= (r1);						\
       (d) = __d;							\
       (co) = __cy;							\
       (sc) = 1; } while (0)

/* Compare two quantities and nullify on condition; Clear destination register.  */
#define PERFORM_COMCLR_SEQ(d, co, sc, r1, r2, ci) \
  ((sc) = (r1) == (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SNE(d, co, sc, r1, r2, ci) \
  ((sc) = (r1) != (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SLTS(d, co, sc, r1, r2, ci) \
  ((sc) = (signed_word) (r1) < (signed_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SGES(d, co, sc, r1, r2, ci) \
  ((sc) = (signed_word) (r1) >= (signed_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SLES(d, co, sc, r1, r2, ci) \
  ((sc) = (signed_word) (r1) <= (signed_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SGTS(d, co, sc, r1, r2, ci) \
  ((sc) = (signed_word) (r1) > (signed_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SLTU(d, co, sc, r1, r2, ci) \
  ((sc) = (unsigned_word) (r1) < (unsigned_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SGEU(d, co, sc, r1, r2, ci) \
  ((sc) = (unsigned_word) (r1) >= (unsigned_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SLEU(d, co, sc, r1, r2, ci) \
  ((sc) = (unsigned_word) (r1) <= (unsigned_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SGTU(d, co, sc, r1, r2, ci) \
  ((sc) = (unsigned_word) (r1) > (unsigned_word) (r2), (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SODD(d, co, sc, r1, r2, ci) \
  ((sc) = (((r1) - (r2)) & 1) != 0, (d) = 0, (co) = (ci))
#define PERFORM_COMCLR_SEVN(d, co, sc, r1, r2, ci) \
  ((sc) = (((r1) - (r2)) & 1) == 0, (d) = 0, (co) = (ci))

/* Logic operations that don't affect carry.  */
#define PERFORM_AND_SEQ(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = (d) == 0)
#define PERFORM_AND_SNE(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = (d) != 0)
#define PERFORM_AND_SLTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = (signed_word) (d) < 0)
#define PERFORM_AND_SGES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = (signed_word) (d) >= 0)
#define PERFORM_AND_SLES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = (signed_word) (d) <= 0)
#define PERFORM_AND_SGTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = (signed_word) (d) == 0)
#define PERFORM_AND_SODD(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = ((d) & 1) != 0)
#define PERFORM_AND_SEVN(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = ((d) & 1) == 0)
#define PERFORM_AND_S(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & (r2), (co) = (ci), (sc) = 1)
#define PERFORM_IOR_SEQ(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = (d) == 0)
#define PERFORM_IOR_SNE(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = (d) != 0)
#define PERFORM_IOR_SLTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = (signed_word) (d) < 0)
#define PERFORM_IOR_SGES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = (signed_word) (d) >= 0)
#define PERFORM_IOR_SLES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = (signed_word) (d) <= 0)
#define PERFORM_IOR_SGTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = (signed_word) (d) == 0)
#define PERFORM_IOR_SODD(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = ((d) & 1) != 0)
#define PERFORM_IOR_SEVN(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = ((d) & 1) == 0)
#define PERFORM_IOR_S(d, co, sc, r1, r2, ci) \
  ((d) = (r1) | (r2), (co) = (ci), (sc) = 1)

#define PERFORM_XOR_SEQ(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = (d) == 0)
#define PERFORM_XOR_SNE(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = (d) != 0)
#define PERFORM_XOR_SLTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = (signed_word) (d) < 0)
#define PERFORM_XOR_SGES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = (signed_word) (d) >= 0)
#define PERFORM_XOR_SLES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = (signed_word) (d) <= 0)
#define PERFORM_XOR_SGTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = (signed_word) (d) > 0)
#define PERFORM_XOR_SODD(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = ((d) & 1) != 0)
#define PERFORM_XOR_SEVN(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = ((d) & 1) == 0)
#define PERFORM_XOR_S(d, co, sc, r1, r2, ci) \
  ((d) = (r1) ^ (r2), (co) = (ci), (sc) = 1)

#define PERFORM_ANDC_SEQ(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = (d) == 0)
#define PERFORM_ANDC_SNE(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = (d) != 0)
#define PERFORM_ANDC_SLTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = (signed_word) (d) < 0)
#define PERFORM_ANDC_SGES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = (signed_word) (d) >= 0)
#define PERFORM_ANDC_SLES(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = (signed_word) (d) <= 0)
#define PERFORM_ANDC_SGTS(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = (signed_word) (d) == 0)
#define PERFORM_ANDC_SODD(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = ((d) & 1) != 0)
#define PERFORM_ANDC_SEVN(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = ((d) & 1) == 0)
#define PERFORM_ANDC_S(d, co, sc, r1, r2, ci) \
  ((d) = (r1) & ~(r2), (co) = (ci), (sc) = 1)

#define PERFORM_LSHIFTR_S(d, co, sc, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >> TRUNC_CNT(r2)), (co) = (ci), (sc) = 1)
#define PERFORM_ASHIFTR_S(d, co, sc, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)), (co) = (ci), (sc) = 1)
#define PERFORM_SHIFTL_S(d, co, sc, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) << TRUNC_CNT(r2)), (co) = (ci), (sc) = 1)
#define PERFORM_ROTATEL_S(d, co, sc, r1, r2, ci) \
 ((d) = TRUNC_CNT(r2) == 0 ? (r1)					\
  : ((r1) << TRUNC_CNT(r2)) | ((r1) >> TRUNC_CNT(BITS_PER_WORD - (r2))),\
  (co) = (ci), (sc) = 1)
#define PERFORM_EXTS1_S(d, co, sc, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 31 >> 31, (co) = (ci), (sc) = 1)
#define PERFORM_EXTS2_S(d, co, sc, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 30 >> 30, (co) = (ci), (sc) = 1)
#define PERFORM_EXTS8_S(d, co, sc, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 24 >> 24, (co) = (ci), (sc) = 1)
#define PERFORM_EXTS16_S(d, co, sc, r1, r2, ci) \
  ((d) = ((signed_word) (r1) >> TRUNC_CNT(r2)) << 16 >> 16, (co) = (ci), (sc) = 1)
#define PERFORM_EXTU1_S(d, co, sc, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >> TRUNC_CNT(r2)) & 1, (co) = (ci), (sc) = 1)
#define PERFORM_EXTU2_S(d, co, sc, r1, r2, ci) \
  ((d) = ((unsigned_word) (r1) >> TRUNC_CNT(r2)) & 3, (co) = (ci), (sc) = 1)

#define IB0(x) x
#define IB1(x) ((x) << 1)
#define IB2(x) ((x) << 2)
#define EB0(x) ((x) & 1)
#define EB1(x) (((x) >> 1) & 1)
#define EB2(x) (((x) >> 2) & 1)

#define PERFORM_ADDC_960(d, co, r1, r2, ci) \
  do { word __d = (r1) + EB1(ci);					\
       word __cy = __d < (r1);						\
       (d) = __d + (r2);						\
       (co) = (IB1(((d) < __d) + __cy)					\
	       | IB0((signed_word) (~((r1) ^ (r2)) & ((d) ^ (r1))) < 0));\
  } while (0)
#define PERFORM_SUBC_960(d, co, r1, r2, ci) \
  do { word __d = (r1) + EB1(ci);					\
       word __cy = __d < (r1);						\
       (d) = __d + ~(r2);						\
       (co) = (IB1(((d) < __d) + __cy)					\
	       | IB0((signed_word) (~((r1) ^ ~(r2)) & ((d) ^ (r1))) < 0));\
  } while (0)
#define PERFORM_SEL_NO_960(d, co, r1, r2, ci) \
  ((d) = (ci) == 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_G_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 1) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_E_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 2) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_GE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 3) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_L_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 4) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_NE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 5) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_LE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 6) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_SEL_O_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 7) != 0 ? (r2) : (r1), (co) = (ci))
#define PERFORM_CONCMPO_960(d, co, r1, r2, ci) \
  ((co) = EB2(ci) == 0							\
   ? ((unsigned_word) (r1) <= (unsigned_word) (r2)) + 1 : (ci))
#define PERFORM_CONCMPI_960(d, co, r1, r2, ci) \
  ((co) = EB2(ci) == 0							\
   ? ((signed_word) (r1) <= (signed_word) (r2)) + 1 : (ci))
#define PERFORM_CMPO_960(d, co, r1, r2, ci) \
  ((co) = (IB0((unsigned_word) (r1) > (unsigned_word) (r2))		\
	   | IB1((r1) == (r2))						\
	   | IB2((unsigned_word) (r1) < (unsigned_word) (r2))))
#define PERFORM_CMPI_960(d, co, r1, r2, ci) \
  ((co) = (IB0((signed_word) (r1) > (signed_word) (r2))			\
	   | IB1((r1) == (r2))						\
	   | IB2((signed_word) (r1) < (signed_word) (r2))))
#define PERFORM_SHIFTL_NT(d, co, r1, r2, ci) \
  ((d) = ((r2) < BITS_PER_WORD ? ((unsigned_word) (r1) << (r2)) : 0), (co) = (ci))
#define PERFORM_LSHIFTR_NT(d, co, r1, r2, ci) \
  ((d) = ((r2) < BITS_PER_WORD ? ((unsigned_word) (r1) >> (r2)) : 0), (co) = (ci))
#define PERFORM_ASHIFTR_NT(d, co, r1, r2, ci) \
  ((d) = ((r2) < BITS_PER_WORD						\
	  ? ((signed_word) (r1) >> (r2))				\
	  : -(signed_word) (r1) < 0), (co) = (ci))
#define PERFORM_ADDO_NO_960(d, co, r1, r2, ci) \
  ((d) = (ci) == 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_G_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 1) != 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_E_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 2) != 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_GE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 3) != 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_L_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 4) != 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_NE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 5) != 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_LE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 6) != 0 ? (r1) + (r2) : (d), (co) = (ci))
#define PERFORM_ADDO_O_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 7) != 0 ? (r1) + (r2) : (d), (co) = (ci))

#define PERFORM_SUBO_NO_960(d, co, r1, r2, ci) \
  ((d) = (ci) == 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_G_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 1) != 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_E_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 2) != 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_GE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 3) != 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_L_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 4) != 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_NE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 5) != 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_LE_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 6) != 0 ? (r1) - (r2) : (d), (co) = (ci))
#define PERFORM_SUBO_O_960(d, co, r1, r2, ci) \
  ((d) = ((ci) & 7) != 0 ? (r1) - (r2) : (d), (co) = (ci))

#define PERFORM_ALTERBIT(d, co, r1, r2, ci) \
  ((d) = (EB1(ci) ? (r1) | ((word) 1 << TRUNC_CNT(r2))			\
		  : (r1) & ~((word) 1 << TRUNC_CNT(r2))),		\
   (co) = (ci))
#define PERFORM_SETBIT(d, co, r1, r2, ci) \
  ((d) = (r1) | ((word) 1 << TRUNC_CNT(r2)), (co) = (ci))
#define PERFORM_CLRBIT(d, co, r1, r2, ci) \
  ((d) = (r1) & ~((word) 1 << TRUNC_CNT(r2)), (co) = (ci))
#define PERFORM_CHKBIT(d, co, r1, r2, ci) \
  ((co) = IB1(((r1) >> TRUNC_CNT(r2)) & 1))
#define PERFORM_NOTBIT(d, co, r1, r2, ci) \
  ((d) = (r1) ^ ((word) 1 << TRUNC_CNT(r2)), (co) = (ci))

enum goal_func
{
#undef	DEF_GOAL
#define DEF_GOAL(SYM,ARITY,NAME,CODE) SYM,
#undef	DEF_SYNONYM
#define DEF_SYNONYM(SYM,NAME)
#include "goal.def"
  LAST_AND_UNUSED_GOAL_CODE
};

enum prune_flags
{
  NO_PRUNE = 0,
  CY_0 = 1,
  CY_1 = 2,
  CY_JUST_SET = 4,
  NULLIFYING_INSN = 8,
};

#if HAS_NULLIFICATION
void synth(insn_t *, int, word *, int, word, int, int, int, int);
void synth_last(insn_t *, int, word *, int, word, int, int, int, int);
void synth_nonskip(insn_t *, int, word *, int, word, int, int, int, int);
void synth_nonskip_last(insn_t *, int, word *, int, word, int, int, int, int);
void synth_condskip(insn_t *, int, word *, int, word, int, int, int, int);
void synth_skip(insn_t *, int, word *, int, word, int, int, int, int);
#else
void synth(insn_t *, int, word *, int, word, int, int, int);
void synth_last(insn_t *, int, word *, int, word, int, int, int);
#endif

void
test_sequence(insn_t *sequence, int n_insns);
int
#if HAS_NULLIFICATION
run_program(insn_t *sequence, int n_insns, word *regs, int arity);
#else
run_program(insn_t *sequence, int n_insns, word *regs);
#endif

extern const char clz_tab[];
extern const char ctz_tab[];
extern const char ff1_tab[];
