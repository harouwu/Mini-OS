# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as

GCCDIR = ../../../gnu-decstation-ultrix/decstation-ultrix/2.95.3/

LDFLAGS = -T script -N
ASFLAGS = -mips2
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)

PATH = $(GCCDIR):/lib:/usr/bin:/bin

CC = $(GCCDIR)gcc -B../../../gnu-decstation-ultrix/
AS = $(GCCDIR)as
LD = $(GCCDIR)ld

CPP = gcc -E
INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt shell matmult sort a b c d e

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.c > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult
a.o: a.c
	$(CC) $(CFLAGS) -c a.c
a: a.o start.o
	$(LD) $(LDFLAGS) start.o a.o -o a.coff
	../bin/coff2noff a.coff a

b.o: b.c
	$(CC) $(CFLAGS) -c b.c
b: b.o start.o
	$(LD) $(LDFLAGS) start.o b.o -o b.coff
	../bin/coff2noff b.coff b

c.o: c.c
	$(CC) $(CFLAGS) -c c.c
c: c.o start.o
	$(LD) $(LDFLAGS) start.o c.o -o c.coff
	../bin/coff2noff c.coff c

d.o: d.c
	$(CC) $(CFLAGS) -c d.c
d: d.o start.o
	$(LD) $(LDFLAGS) start.o d.o -o d.coff
	../bin/coff2noff d.coff d

e.o: e.c
	$(CC) $(CFLAGS) -c e.c
e: e.o start.o
	$(LD) $(LDFLAGS) start.o e.o -o e.coff
	../bin/coff2noff e.coff e
