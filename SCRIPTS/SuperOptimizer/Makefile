# Makefile for GNU superoptimizer

MAXCOST = 3
EXTRA = -shifts -extracts

CC = gcc
DBG = -g
OPT = -O3
CFLAGS = $(CPU) $(OPT) $(DBG)
ALL_MACHINES =	superopt-sparc \
		superopt-power \
		superopt-powerpc \
		superopt-m88000 \
		superopt-am29k \
		superopt-mc68000 \
		superopt-mc68020 \
		superopt-i386 \
		superopt-i960a \
		superopt-i960b \
		superopt-pyr \
		superopt-alpha \
		superopt-sh \
		superopt-hppa

OBJS	= superopt.o
SRCS	= superopt.c
HDRS	= run_program.def insn.def goal.def superopt.h version.h longlong.h
BINDIR	= /usr/local/bin
INSTALL	= install -c
FILES	= README COPYING Makefile TODO ChangeLog superopt.c synth.def $(HDRS)

VERSION = `sed 's,char \*version_string = "\([0-9.]*\)";,\1,' < version.h`

superopt: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o superopt $(SRCS)

clean:
	rm -f $(OBJS) superopt $(ALL_MACHINES) *.tmp

install: superopt
	$(INSTALL) superopt $(BINDIR)/superopt

all: $(ALL_MACHINES)

install-all: all
	for x in $(ALL_MACHINES); do $(INSTALL) $$x $(BINDIR)/$$x; done

run-all: sparc.res power.res powerpc.res m88000.res am29k.res mc68000.res \
	mc68020.res i386.res i960a.res i960b.res pyr.res alpha.res sh.res \
	hppa.res
	@echo "Done!"

superopt-sparc: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DSPARC -o superopt-sparc $(SRCS)
sparc.res: superopt-sparc
	./superopt-sparc -all $(EXTRA) -max $(MAXCOST) -as >sparc.tmp 2>&1
	mv sparc.tmp sparc.res

superopt-power: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DPOWER -o superopt-power $(SRCS)
power.res: superopt-power
	./superopt-power -all $(EXTRA) -max $(MAXCOST) -as >power.tmp 2>&1
	mv power.tmp power.res

superopt-powerpc: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DPOWERPC -o superopt-powerpc $(SRCS)
powerpc.res: superopt-powerpc
	./superopt-powerpc -all $(EXTRA) -max $(MAXCOST) -as >powerpc.tmp 2>&1
	mv powerpc.tmp powerpc.res

superopt-m88000: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DM88000 -o superopt-m88000 $(SRCS)
m88000.res: superopt-m88000
	./superopt-m88000 -all $(EXTRA) -max $(MAXCOST) -as >m88000.tmp 2>&1
	mv m88000.tmp m88000.res

superopt-am29k: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DAM29K -o superopt-am29k $(SRCS)
am29k.res: superopt-am29k
	./superopt-am29k -all $(EXTRA) -max $(MAXCOST) -as >am29k.tmp 2>&1
	mv am29k.tmp am29k.res

superopt-mc68000: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DMC68000 -o superopt-mc68000 $(SRCS)
mc68000.res: superopt-mc68000
	./superopt-mc68000 -all $(EXTRA) -max $(MAXCOST) -as >mc68000.tmp 2>&1
	mv mc68000.tmp mc68000.res

superopt-mc68020: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DMC68020 -o superopt-mc68020 $(SRCS)
mc68020.res: superopt-mc68020
	./superopt-mc68020 -all $(EXTRA) -max $(MAXCOST) -as >mc68020.tmp 2>&1
	mv mc68020.tmp mc68020.res

superopt-i386: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DI386 -o superopt-i386 $(SRCS)
i386.res: superopt-i386
	./superopt-i386 -all $(EXTRA) -max $(MAXCOST) -as >i386.tmp 2>&1
	mv i386.tmp i386.res

superopt-i960a: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DI960 -o superopt-i960a $(SRCS)
i960a.res: superopt-i960a
	./superopt-i960a -all $(EXTRA) -max $(MAXCOST) -as >i960a.tmp 2>&1
	mv i960a.tmp i960a.res

superopt-i960b: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DI960B -o superopt-i960b $(SRCS)
i960b.res: superopt-i960b
	./superopt-i960b -all $(EXTRA) -max $(MAXCOST) -as >i960b.tmp 2>&1
	mv i960b.tmp i960b.res

superopt-pyr: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DPYR -o superopt-pyr $(SRCS)
pyr.res: superopt-pyr
	./superopt-pyr -all $(EXTRA) -max $(MAXCOST) -as >pyr.tmp 2>&1
	mv pyr.tmp pyr.res

superopt-alpha: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DALPHA -o superopt-alpha $(SRCS)
alpha.res: superopt-alpha
	./superopt-alpha -all $(EXTRA) -max $(MAXCOST) -as >alpha.tmp 2>&1
	mv alpha.tmp alpha.res

superopt-sh: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DSH -o superopt-sh $(SRCS)
sh.res: superopt-sh
	./superopt-sh -all $(EXTRA) -max $(MAXCOST) -as >sh.tmp 2>&1
	mv sh.tmp sh.res

superopt-hppa: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -DHPPA -o superopt-hppa $(SRCS)
hppa.res: superopt-hppa
	./superopt-hppa -all $(EXTRA) -max $(MAXCOST) -as >hppa.tmp 2>&1
	mv hppa.tmp hppa.res

dist:
	mkdir superopt-$(VERSION)
	ln $(FILES) superopt-$(VERSION)
	tar cf - superopt-$(VERSION) | gzip --best > superopt-$(VERSION).tar.gz
	rm -rf superopt-$(VERSION)
