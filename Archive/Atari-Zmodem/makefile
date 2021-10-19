#
# Makefile for Mark Williams C
#	Version 2.00 (or higher) required.
#	Please ignore warnigs about a constant being promoted to long
#	and about some unused variables.
#	
# 	Edit config.h before make'ing.
#
#	Jwahar Bammi
#
#

SRC = common.c rz.c sz.c transfer.c util.c main.c tyme.c zm.c fileio.c \
hi5025.s phone.c

OBJ =  fileio.o zm.o util.o rz.o sz.o common.o tyme.o transfer.o main.o \
hi5025.o phone.o

CFLAGS = -O -V -VPEEP -DMWC=1
LDFLAGS = -s -x -V

zmdm.prg : $(OBJ)
	cc -O -o zmdm.prg $(OBJ) $(LDFLAGS)

$(OBJ) : common.h zmdm.h decl.h config.h
transfer.o : expandar.c

clean:
	rm *.o
