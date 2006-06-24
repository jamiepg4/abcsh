#
# Makefile for Amiga Bourne Compatible Shell.
#

.PHONY: all clean mostlyclean clibhackclean noclibhack debug newlib newlib-debug

CC = ppc-amigaos-gcc
STRIP = ppc-amigaos-strip

CFLAGS = -mcrt=clib2 -O2 -DNDEBUG
LDFLAGS = -mcrt=clib2

#use "make CLIBHACK=-UCLIBHACK" to use IDOS io code
CLIBHACK = -DCLIBHACK

DEFS = -DAMIGA -DPOSIXLY_CORRECT -DHISTORY -DAUTOINIT -D__STDC_VERSION__=199901L $(CLIBHACK)
LIBS = -lnet -lm -lunix

SRCS = amigaos.c alloc.c c_ksh.c c_sh.c c_test.c environ.c eval.c exec.c \
	expr.c history.c io.c jobs.c lex.c main.c misc.c missing.c path.c \
	shf.c sigact.c syn.c table.c trap.c tree.c tty.c var.c version.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

all: sh
	@echo done.

sh: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(STRIP) -R.comment $@

%.o : %.c
	$(CC) -MM -MP $(INCDIRS) $< >$*.d
	$(CC) -c $(DEFS) -I. $(CFLAGS) $<

clean:
	rm -f sh $(OBJS) $(DEPS)

mostlyclean: clean

clibhackclean:
	rm -f amigaos.o c_sh.o c_ksh.o exec.o eval.o

noclibhack: clibhackclean
	$(MAKE) -f Makefile CLIBHACK=-UCLIBHACK

debug: clean
	$(MAKE) -f Makefile.debug

newlib: clean
	$(MAKE) -f Makefile.newlib

newlib-debug: clean
	$(MAKE) -f Makefile.newlib_debug

-include $(SRCS:.c=.d)
