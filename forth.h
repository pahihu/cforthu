/*
 * forth.h -- define function numbers for primitives, and other constants,
 * externals, and globals used in forth.c and prims.c
 */

#define EXECUTE		0
#define LIT		1
#define BRANCH		2
#define ZBRANCH		3
#define PLOOP		4
#define PPLOOP		5
#define PDO		6
#define I		7
#define R		58
#define DIGIT		8
#define PFIND		9
#define ENCLOSE		10
#define KEY		11
#define PEMIT		12
#define QTERMINAL	13
#define CMOVE		14
#define USTAR		15
#define USLASH		16
#define AND		17
#define OR		18
#define XOR		19
#define SPFETCH		20
#define SPSTORE		21
#define RPFETCH		22
#define RPSTORE		23
#define SEMIS		24
#define LEAVE		25
#define TOR		26
#define FROMR		27
#define ZEQ		28
#define ZLESS		29
#define PLUS		30
#define DPLUS		31
#define MINUS		32
#define DMINUS		33
#define OVER		34
#define DROP		35
#define SWAP		36
#define DUP		37
#define TDUP		38
#define PSTORE		39
#define TOGGLE		40
#define FETCH		41
#define CFETCH		42
#define TFETCH		43
#define STORE		44
#define CSTORE		45
#define TSTORE		46
#define DOCOL		47
#define DOCON		48
#define DOVAR		49
#define DOUSE		50
#define SUBTRACT	51
#define EQUAL		52
#define NOTEQ		53
#define LESS		54
#define ROT		55
#define DODOES		56
#define DOVOC		57
/* 58 is above */
#define ALLOT		59
#define PBYE		60
#define TRON		61
#define TROFF		62
#define DOTRACE		63
#define PRSLW		64
#define PSAVE		65
#define PCOLD		66

/* memory */
#define GULPFRQ		256	/* if mem[LIMIT] - dp < GULPFRQ, then get */
#define GULPSIZE	1024	/* a block of GULPSIZE words		  */

/*
 * User variables and other locations
 */

#define S0	UP+0		/* csp when stack is empty */
#define R0	UP+1		/* rsp when r stack is empty */
#define TIB	UP+2		/* Terminal Input Buffer location */
#define WIDTH	UP+3		/* screen width */
#define WARNING	UP+4		/* print messages? */
#define FENCE	UP+5		/* can not forget below this mark */
#define DP	UP+6		/* points to first unallocated word */
#define VOCLINK UP+7		/* vocabulary link */

/* GLOBALS */

/* STACK POINTERS are registers of our FORTH machine. They, like everything
   else, point into memory (mem[]). They are read by sp@ and rp@, set by sp!
   and rp!. They are initialized by COLD. */

extern UCell csp;
extern UCell rsp;

/* This variable is all-important. It will be set to the top of the 
   data area by sbrk, and more memory will be allocated. All memory is
   addressed as a subscript to this address -- mem[0] is the first memory 
   element, mem[1] is second, and so on. 
*/

extern Cell *mem;	/* points to the number of bytes in mem[0], as read
			   from COREFILE at startup */

/* two more machine registers: the interpretive pointer */
extern UCell ip;	        /* for an explanation of these, look in */
extern UCell w;	                /* interp.doc */

extern int trace, debug;	/* global for tracing in next() */
extern int tracedepth, breakenable, qtermflag, forceip, nobuf;
extern Cell breakpoint;
extern FILE *blockfile;
extern long bfilesize;
extern char *bfilename;
extern char *cfilename;
extern char *sfilename;

/* stack manipulation */
extern void push(Word);
extern Cell pop();
extern void rpush(Word);
extern Cell rpop();
extern void errexit(char*, ...);
