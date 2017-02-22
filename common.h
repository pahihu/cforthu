/*
 * This is common.h -- the defines which are common to both nf.c and forth.c.
 * These include the name of the SAVEFILE (the file which nf.c creates,
 * and the default image which f.c loads), and all those boundaries for
 * memory areas, like UP, USER_DEFAULTS, etc.
 */

/*
 * NOTE THAT THIS FORTH IMPLENTATION REQUIRES int TO BE TWICE THE SIZE OF short
 */

#define TRUE 1
#define FALSE 0

/*
   TWEAKING: define TRACE to allow tracing, BREAKPOINT to allow breakpoints.
   Each of these takes up time in the inner interpreter, so if you are
   not debugging, take them out. Without TRACE, the DOTRACE primitive will
   still work, but the TRON primitive will have no effect.
*/

#define TRACE
#define BREAKPOINT

/* external files */

#define COREFILE "forth.cor"	/* used for input to f.c, output from nf.c */
#define DICTFILE "forth.dic"	/* used for input to nf.c */
#define MAPFILE "forth.map"	/* used for dump-output from nf.c */
#define DUMPFILE "forth.dmp"	/* used for dump-output from f.c */
#define BLOCKFILE "forth.blk"	/* used for block i/o */
#define SAVEFILE "forth.sav"	/* used by (SAVE) primitive */

/* MEMORY ALLOCATION CONSTANTS */

/* Set INITMEM to the size of the largest FORTH model you want nf to create.
   This can be just barely enough (within GULPFRQ words) to hold the initial 
   FORTH image, or it can be the maximum size you will ever want. Somewhere in
   between is best, so you don't fragment memory with realloc() calls right
   away. */

#define INITMEM (13*1024)	/* 13K holds the distribution forth.dict */
  
/* set MAXMEM to the MOST MEMORY YOU EVER WANT ALLOCATED TO FORTH. FORTH will
   never allocate more than MAXMEM*sizeof(short) for the FORTH memory image.
   Note that other functions, like open, read, and write, allocate memory
   transparent to the forth system. MAXMEM will not affect these. Also,
   note that realloc is used to grow the FORTH image, and LARGE CHUNKS of
   fragmented memory can result. If you want to keep a tight rein on things,
   set MAXMEM to the same number as INITMEM, and the FORTH memory image will
   be fixed at that many SHORTs, with no later allocations, and therefore
   no fragmenting.
	A value of 0 for MAXMEM means "allocate as much as you want" -- 
   useful on virtual-memory machines. Also note that each malloc and realloc
   is checked for success (of course), so MAXMEM is truly a maximal limit.
	NOTE THAT MODELS OF GREATER THAN 32K MAY CRASH BECAUSE OF SIGNED
   VALUES. THIS HAS NOT BEEN ADEQUATELY TESTED.
*/

#define MAXMEM 0

/* set NSCR to the number of disk blocks from you want to keep in FORTH memory
   at any time. If your disks are fast enough, you might want a low number
   like 3. If you have lots of memory, you might want something like 10.
   In any case, this number MUST BE AT LEAST 2. */

#define NSCR 5	/* MUST BE AT LEAST 2 */

/* end of implementation-dependent DEFINEs. */

/* define bits for the first byte of each word */
#define MSB 0x80		/* says this is first byte */
#define IMMEDIATE 0x40		/* Says this word is immediate */
#define SMUDGE 0x20		/* on = you can't find this word */

#define MAXWIDTH 0x20		/* Maximum length of a word */

#define KBBUFF 1024		/* one disk-quantum */
#define US 32			/* words needed for user variables */
#define CO (KBBUFF+4)
				/* size of a disk buffer w/4 words overhead */
#define NBUF NSCR		/* number of disk buffers, at 1 to a screen */

/* Memory Management boundaries -- each name refers to the FIRST location of
   the indicated field Some fields are nested, and I have tried to show the
   nesting nature in the defines. */

#define ORIGIN 0		/* the Origin of this system is zero */
#define ORIG ORIGIN		/* another word for ORIGIN */
#define SCRATCHSIZE 16		/* From ORIGIN to ORIGIN+SCRATCHSIZE is scratch
				   space which is saved across saves: see the
				   definition of this space below */
#define USER_DEFAULTS (ORIGIN+SCRATCHSIZE)	/* 16 */
				/* start of user variable initial-values space
				   -- it's DEFS-SIZE bytes long */
#define DEFS_SIZE 8		/* words in the USER DEFAULTS area */
#define UP (USER_DEFAULTS+DEFS_SIZE)	/* User var space, US bytes long */
#define TIB_START (UP+US)	/* Terminal input buffer, same size as a
				   disk buffer (KBBUFF words), starts after
				   user variables */
#define TIB_END (TIB_START + KBBUFF)
#define CS_SIZE 128		/* words in the Computation Stack */
#define RS_SIZE 256		/* words in the Return Stack */
#define INITS0 (TIB_START+KBBUFF+CS_SIZE) /* c. stack grows down CSS words,
				   bangs into end of TIB */
#define INITR0 (INITS0+RS_SIZE)	/* Return stack grows down RSS words, bangs
				   into INITS0. */
#define BUF1 INITR0		/* buffers start right after r. stack */
#define DPBASE (BUF1+(NBUF*CO))	/* Dictionary starts just past last buffer */

/* low-core definitions */
#define LIMIT 0			/* mem[LIMIT] tells the size of core */
#define COLDIP 1		/* mem[COLDIP] holds the CFA of ABORT */
		/* you can set ip=mem[COLDIP] and call next() to start */

/* these locations define the warm-start machine state: if you save the FORTH
   memory image, then restart it, execution will start up with these values.
   This save/restore system is not implemented, so leave mem[SAVEDIP] = 0. */

#define SAVEDIP 2		/* mem[SAVEDIP] = 0 for newly-generated
				   systems, or the IP for a saved system */
#define SAVEDSP 3		/* restored when SAVEDIP != 0 */
#define SAVEDRP 4		/* ditto */

#define ABORTIP 5		/* need this to recover from ^C */
