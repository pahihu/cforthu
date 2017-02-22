CC = cc
CFLAGS = -m32 -O1 #-DDEBUG=1
EXE = # .exe

test:		forth.cor forth$(EXE)

forth$(EXE):	forth.o prims.o
		$(CC) $(CFLAGS) -o forth$(EXE) forth.o prims.o

forth.o:	forth.c common.h forth.h prims.h
		$(CC) $(CFLAGS) -c forth.c

prims.o:	prims.c forth.h prims.h
		$(CC) $(CFLAGS) -c prims.c

all:		forth$(EXE) forth.cor lin2blk$(EXE) blk2lin$(EXE)

nf$(EXE):	nf.o lex.yy.o
		$(CC) $(CFLAGS) -o nf$(EXE) nf.o lex.yy.o

nf.o:		nf.c forth.lex.h common.h
		$(CC) $(CFLAGS) -c nf.c

lex.yy.o:	lex.yy.c forth.lex.h
		$(CC) $(CFLAGS) -c lex.yy.c

lex.yy.c:	forth.lex
		lex forth.lex
		$(RM) lex.tmp
		sed "s/int yylex (void)/TOKEN *yylex (void)/g" lex.yy.c > lex.tmp
		mv -f lex.tmp lex.yy.c

forth.cor:	nf$(EXE) forth.dic
		nf$(EXE) < forth.dic

# lin2blk: convert a line file to a block file.
# Usage: lin2blk < linefile > blockfile
lin2blk$(EXE):	lin2blk.c
		$(CC) $(CFLAGS) -o lin2blk$(EXE) lin2blk.c

# blk2lin: convert a block file to a line file.
# Usage: blk2lin < blockfile > linefile
blk2lin$(EXE):	blk2lin.c
		$(CC) $(CFLAGS) -o blk2lin$(EXE) blk2lin.c

# forth.lin and forth.blk are not included here, because you can't tell
# which one is more recent. To make one from the other, use blk2lin and
# lin2blk.

clean:
		$(RM) forth.cor forth.dmp forth.map
		$(RM) lex.yy.c
		$(RM) *.o nf$(EXE) forth$(EXE) lin2blk$(EXE) blk2lin$(EXE)
