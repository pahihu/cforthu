/*
 * forth.c
 * 
 * Portable FORTH interpreter in C
 *
 * Author: Allan Pratt, Indiana University (iuvax!apratt)
 *         Spring, 1984
 * References: 8080 and 6502 fig-FORTH source listings (not the greatest refs
 *         in the world...)
 *
 * This program is intended to be compact, portable, and pretty complete.
 * It is also intended to be in the public domain, and distribution should
 * include this notice to that effect.
 *
 * This file contains the support code for all interpreter functions.
 * the file prims.c contains code for the C-coded primitives, and the
 * file forth.h connects the two with definitions.
 *
 * The program nf.c generates a new forth.core file from the dictionary
 * forth.dict, using common.h to tie it together with this program.
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>	/* only for isxdigit */

#include "common.h"

#include "forth.h"

#include "prims.h"	/* macro-defined primitives */

/* declare globals which are defined in forth.h */

unsigned short csp, rsp, ip, w;
short *mem;
int trace, tracedepth, debug, breakenable, breakpoint, qtermflag, forceip;
int nobuf;
FILE *blockfile;
long bfilesize;
char *bfilename;	/* block file name (change with -f ) */
char *cfilename;	/* core file name  (change with -l ) */
char *sfilename;	/* save file name  (change with -s ) */

/* forward declarations */
void memdump();
void dotrace();
void dobreak();
void usage();
int xtoi(char*);
void initsignals();
void getblockfile();

/*
             ----------------------------------------------------
                               SYSTEM FUNCTIONS
             ----------------------------------------------------
*/

void errexit(char *s, ...)		/* An error occurred -- clean up (?) and
				   exit. */
{
    va_list ap;
    va_start(ap,s);
    vprintf(s,ap); 
    va_end(ap);

    printf("ABORT FORTH!\nDumping to %s... ",DUMPFILE);
    fflush(stdout);
    memdump();
    puts("done.");
    exit(1);
}

void Callot (n)			/* allot n words in the dictionary */
short n;
{
    unsigned newsize;

    mem[DP] += n;			/* move DP */
    if (mem[DP] + GULPFRQ > mem[LIMIT]) {	/* need space */
	newsize = mem[DP] + GULPSIZE;
	if (newsize > MAXMEM && MAXMEM)
		errexit("ATTEMPT TO GROW PAST MAXMEM (%d) WORDS\n",MAXMEM);

	mem = (short *)realloc((char *)mem, newsize*sizeof(*mem));
	if (mem == NULL)
		errexit("REALLOC FAILED\n");
	mem[LIMIT] = newsize;
    }
}

void push(v)			/* push value v to cstack */
int v;
{
    if (csp <= TIB_END)
	errexit("PUSH TO FULL CALC. STACK\n");
    mem[--csp] = (short) v;
}

short pop()			/* pop a value from comp. stack, and return
				   it as the value of the function */
{
    if (csp >= INITS0) {
	puts("Empty Stack!");
	return 0;
    }
    return (mem[csp++]);
}

void rpush(v)
int v;
{
    if (rsp <= INITS0)
	errexit("PUSH TO FULL RETURN STACK");
    mem[--rsp] = (short) v;
}

short rpop()
{
    if (rsp >= INITR0)
	errexit("POP FROM EMPTY RETURN STACK!");
    return (mem[rsp++]);
}

int pkey()		/* (KEY) -- wait for a key & return it */
{
    int c;
    if ((c = getchar()) == EOF) errexit("END-OF-FILE ENCOUNTERED");
    return(c);
}

void pqterm()			/* (?TERMINAL): 
					return true if BREAK has been hit */
{
	if (qtermflag) {
		push(TRUE);
		qtermflag = FALSE;	/* this influences ^C handling */
	}
	else push(FALSE);
}

void pemit()				/* (EMIT): c --	emit a character */
{
    putchar(pop() & 0x7f);	/* stdout is unbuffered */
}

void next()			/* instruction processor: control goes here
				   almost right away, and cycles through here
				   until you leave. */

/* 
 * This is the big kabloona. What it does is load the value at mem[ip]
 * into w, increment ip, and invoke prim. number w. This implies that
 * mem[ip] is the CFA of a word. What's in the CF of a word is the number
 * of the primitive which should be executed. For a word written in FORTH,
 * that primitive is "docol", which pushes ip to the return stack, then
 * uses w+2 (the PFA of the word) as the new ip.  See "interp.doc" for
 * more.
 */

/*
 * There is an incredible hack going on here: the SPECIAL CASE mentioned in
 * the code is for the word EXECUTE, which must set W itself and jump INSIDE
 * the "next" loop, by-passing the first instruction. This has been made a
 * special case: if the primitive to execute is zero, the special case is
 * invoked, and the code for EXECUTE is put right in the NEXT loop. For this
 * reason, "EXECUTE" MUST BE THE FIRST WORD IN THE DICTIONARY.
 */
{
    short p;
	
    while (1) {
	if (forceip) {		/* force ip to this value -- used by sig_int */
		ip = forceip;
		forceip = FALSE;
	}
#ifdef TRACE
	if (trace) dotrace();
#endif /* TRACE */

#ifdef BREAKPOINT
	if (breakenable && ip == breakpoint) dobreak();
#endif /* BREAKPOINT */

	w = mem[ip];
	ip++;
				/* w, mem, and ip are all global. W is now
				   a POINTER TO the primitive number to 
				   execute, and ip points to the NEXT thread to
				   follow. */

next1:				/* This is for the SPECIAL CASE */
	p = mem[w];		/* p is the actual number of the primitive */
	if (p == 0) {		/* SPECIAL CASE FOR EXECUTE! */
	    w = pop();		/* see above for explanation */
	    goto next1;
	}
	/* else */
	switch(p) {
	case LIT	:  lit(); break;
	case BRANCH	:  branch(); break;
	case ZBRANCH	:  zbranch(); break;
	case PLOOP	:  ploop(); break;
	case PPLOOP	:  pploop(); break;
	case PDO	:  pdo(); break;
	case I		:  i(); break;
	case R		:  r(); break;
	case DIGIT	:  digit(); break;
	case PFIND	:  pfind(); break;
	case ENCLOSE	:  enclose(); break;
	case KEY	:  key(); break;
	case PEMIT	:  pemit(); break;
	case QTERMINAL	:  qterminal(); break;
	case CMOVE	:  cmove(); break;
	case USTAR	:  ustar(); break;
	case USLASH	:  uslash(); break;
	case AND	:  and(); break;
	case OR		:  or(); break;
	case XOR	:  xor(); break;
	case SPFETCH	:  spfetch(); break;
	case SPSTORE	:  spstore(); break;
	case RPFETCH	:  rpfetch(); break;
	case RPSTORE	:  rpstore(); break;
	case SEMIS	:  semis(); break;
	case LEAVE	:  leave(); break;
	case TOR	:  tor(); break;
	case FROMR	:  fromr(); break;
	case ZEQ	:  zeq(); break;
	case ZLESS	:  zless(); break;
	case PLUS	:  plus(); break;
	case DPLUS	:  dplus(); break;
	case MINUS	:  minus(); break;
	case DMINUS	:  dminus(); break;
	case OVER	:  over(); break;
	case DROP	:  drop(); break;
	case SWAP	:  swap(); break;
	case DUP	:  dup(); break;
	case TDUP	:  tdup(); break;
	case PSTORE	:  pstore(); break;
	case TOGGLE	:  toggle(); break;
	case FETCH	:  fetch(); break;
	case CFETCH	:  cfetch(); break;
	case TFETCH	:  tfetch(); break;
	case STORE	:  store(); break;
	case CSTORE	:  cstore(); break;
	case TSTORE	:  tstore(); break;
	case DOCOL	:  docol(); break;
	case DOCON	:  docon(); break;
	case DOVAR	:  dovar(); break;
	case DOUSE	:  douse(); break;
	case SUBTRACT	:  subtract(); break;
	case EQUAL	:  equal(); break;
	case NOTEQ	:  noteq(); break;
	case LESS	:  less(); break;
	case ROT	:  rot(); break;
	case DODOES	:  dodoes(); break;
	case DOVOC	:  dovoc(); break;
	case ALLOT	:  allot(); break;
	case PBYE	:  pbye(); break;
	case TRON	:  tron(); break;
	case TROFF	:  troff(); break;
	case DOTRACE	:  dotrace(); break;
	case PRSLW	:  prslw(); break;
	case PSAVE	:  psave(); break;
	case PCOLD	:  pcold(); break;
	default		:  errexit("Bad execute-code %d\n",p); break;
	}
    }
}

void dotrace()
{
	short worka, workb, workc;
	putchar('\n');
	if (tracedepth) {		/* show any stack? */
		printf("sp: %04x (", csp);
		worka = csp;
		for (workb = tracedepth; workb; workb--)
			printf("%04x ",(unsigned short) mem[worka++]);
		putchar(')');
	}
	printf(" ip=%04x ",ip);

	if (mem[R0]-rsp < RS_SIZE && mem[R0] - rsp > 0) /* if legal rsp */
	    for (worka = mem[R0]-rsp; worka; worka--) { /* indent */
		putchar('>');
		putchar(' ');
	    }
	worka = mem[ip] - 3;		/* this is second-to-last letter, or
					   the count byte */
	while (!(mem[worka] & 0x80)) worka--;	/* skip back to count byte */
	workc = mem[worka] & 0x2f;		/* workc is count value */
	worka++;
	while (workc--) putchar(mem[worka++] & 0x7f);
	fflush(stdout);
	if (debug) {		/* wait for \n -- any other input will dump */
		char buffer[10];
		if (*gets(buffer) != '\0') {
			printf("dumping core... ");
			fflush(stdout);
			memdump();
			puts("done.");
		}
	}
}

#ifdef BREAKPOINT
void dobreak()
{
	int temp;
	puts("Breakpoint.");
	printf("Stack pointer = %x:\n",csp);
	for (temp = csp; temp < INITS0; temp++)
		printf("\t%04x",mem[temp]);
	putchar('\n');
}
#endif /* BREAKPOINT */

int main(argc,argv)
int argc;
char *argv[];
{
	FILE *fp;
	unsigned short size;
	int i = 1;

	cfilename = COREFILE;	/* "forth.core" */
	bfilename = BLOCKFILE;	/* "forth.block" */
	sfilename = SAVEFILE;	/* "forth.newcore" */
	trace = debug = breakenable = nobuf = 0;

	while (i < argc) {
		if (*argv[i] == '-') {
			switch (*(argv[i]+1)) {
#ifdef TRACE
			case 'd':			/* -d[n] */
				debug = 1;	/* ...and fall through */
			case 't':			/* -t[n] */
				trace = TRUE;
				if (argv[i][2])
					tracedepth = (argv[i][2] - '0');
				else tracedepth = 0;
				break;
#else /* !TRACE */
			case 'd':
			case 't':
				fprintf(stderr,
		"Must compile with TRACE defined for -t or -d\n");
				break;
#endif /* TRACE */
			case 'c': if (++i == argc) usage(argv[0]);
				  cfilename = argv[i];		/* -c file */
				  break;
			case 's': if (++i == argc) usage(argv[0]);
				  sfilename = argv[i];		/* -s file */
				  break;
#ifdef BREAKPOINT
			case 'p': if (++i == argc) usage(argv[0]);
				  breakenable = TRUE;	/* -p xxxx */
				  breakpoint = xtoi(argv[i]);
				  break;
#else /* !BREAKPOINT */
			case 'p': fprintf(stderr,
		"Must compile with BREAKPOINT defined for -p");
				  break;
#endif /* BREAKPOINT */
			case 'b': if (++i == argc) usage();
				  bfilename = argv[i]; /* -b blockfile */
				  break;
			case 'n': nobuf = TRUE;
				  break;
			default: usage(argv[0]);
				 exit(1);
			}
		}
		else usage(argv[0]);		/* not a dash */
		i++;
	}

	if ((fp = fopen(cfilename,"r")) == NULL) {
		fprintf(stderr,"Forth: Could not open %s\n", cfilename);
		exit(1);
	}
	if (fread(&size, sizeof(size), 1, fp) != 1) {
		fprintf(stderr,"Forth: %s is empty.\n",cfilename);
		exit(1) ;
	}

	if ((mem = (short *)calloc(size, sizeof(*mem))) == NULL) {
		fprintf(stderr, "Forth: unable to malloc(%d,%d)\n",
			size, (int)sizeof(*mem));
		exit(1);
	}

	mem[LIMIT] = size;

	if (fread(mem+1, sizeof(*mem), size-1, fp) != size-1) {
		fprintf(stderr, "Forth: not %d bytes on %s.\n",
			size, cfilename);
		exit(1);
	}

	fclose(fp);

	initsignals();

	getblockfile();

	if (!nobuf) setbuf(stdout,NULL);

	if ((ip = mem[SAVEDIP])) {	/* if savedip != 0, that is */
		csp = mem[SAVEDSP];
		rsp = mem[SAVEDRP];
		puts("restarting a saved FORTH image");
	}
	else {
		ip = mem[COLDIP];	/* this is the ip passed from nf.c */
			/* ip now points to a word holding the CFA of COLD */
		rsp = INITR0;		/* initialize return stack */
		csp = INITS0;
	}
	next();
	/* never returns */
}

void usage(s)
char *s;
{
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "%s [-t[n]] [-d[n]] [-p xxxx] [-n]\n",s);
	fputs("\t[-c corename] [-b blockname] [-s savename]\n", stderr);
	fputs("Where:\n", stderr);
	fputs("-t[n]\t\tsets trace mode\n", stderr);
	fputs("-d[n]\t\tsets trace mode and debug mode (waits for newline)",
                stderr);
	fputs(
"\t\t [n] above sets stack depth to display. Single digit, 0-9. Default 0.\n",
                stderr);
	fputs(
"-p xxxx\t\tsets a breakpoint at xxxx (in hex), shows stack when reached\n",
                stderr);
	fputs("-n\t\tleaves stdout line-buffered\n", stderr);
	fprintf(stderr,
"-c corename\tuses corename as the core image (default %s without -c)\n",
		COREFILE);
	fprintf(stderr,
"-b blockname\tuses blockname as the blockfile (default %s without -b)\n",
		BLOCKFILE);
	fprintf(stderr,
"-s savename\tuses savename as the save-image file (default %s without -s)\n",
		SAVEFILE);
}

void memdump()		/* dump core. */
{
	int i;	/* top of RAM */
	int temp, tempb, firstzero, nonzero;
	char chars[9], outline[80], tstr[6];
	FILE *dumpfile;

	dumpfile = fopen(DUMPFILE,"w");

	fprintf(dumpfile,
		"CSP = 0x%x  RSP = 0x%x  IP = 0x%x  W = 0x%x  DP = 0x%x\n",
		csp, rsp, ip, w, mem[DP]);

	for (temp = 0; temp < mem[LIMIT]; temp += 8) {
		nonzero = FALSE;
		sprintf(outline, "%04x:", temp);
		for (i=temp; i<temp+8; i++) {
			sprintf(tstr," %04x", (unsigned short)mem[i]);
			strcat(outline, tstr);
			tempb = mem[i] & 0x7f;
			if (tempb < 0x7f && tempb >= ' ')
				chars[i%8] = tempb;
			else
				chars[i%8] = '.';
			nonzero |= mem[i];
		}
		if (nonzero) {
			fprintf(dumpfile,"%s %s\n",outline,chars);
			firstzero = TRUE;
		}
		else if (firstzero) {
			fprintf(dumpfile, "----- ZERO ----\n");
			firstzero = FALSE;
		}
	}
	fclose(dumpfile);
}

/* here is where ctype.h is used */

int xtoi(s)
char *s;
{				/*  convert hex ascii to integer */
    int temp = 0;

    while (isxdigit (*s)) {	/* first non-hex char ends */
	temp <<= 4;		/* mul by 16 */
	if (isupper (*s))
	    temp += (*s - 'A') + 10;
	else
	    if (islower (*s))
		temp += (*s - 'a') + 10;
	    else
		temp += (*s - '0');
	s++;
    }
    return temp;
}

/*
 * Interrupt (^C) handling: If the user hits ^C once, the next pqterm call
 * will return TRUE. If he hits ^C again before pqterm is called, there will
 * be a forced jump to ABORT next time we hit next(). If it is a primitive
 * that is caught in an infinite loop, this won't help any.
 */

void sig_int()
{
	if (qtermflag) {		/* second time? */
		forceip = mem[ABORTIP];	/* checked each time through next */
		qtermflag = FALSE;
		trace = FALSE;		/* stop tracing; reset */
	}
	else qtermflag = TRUE;
}

void initsignals()
{
	signal(SIGINT,sig_int);
}

void getblockfile()
{
	/* recall that opening with mode "a+" opens for reading and writing */
	/* with the pointer positioned at the end; this is so ftell returns */
	/* the size of the file.					    */

	if ((blockfile = fopen(bfilename, "r+")) == NULL) /* M000 */
		errexit("Can't open blockfile \"%s\"\n", bfilename);
        fseek(blockfile, 0L, SEEK_END); /* M000 */
	bfilesize = ftell(blockfile);

	printf("Block file has %d blocks.\n",(int) (bfilesize/1024) - 1);
}
