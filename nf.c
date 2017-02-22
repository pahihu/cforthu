/* nf.c -- this program can be run to generate a new environment for the
 * FORTH interpreter forth.c. It takes the dictionary from the standard input.
 * Normally, this dictionary is in the file "forth.dict", so 
 *	nf < forth.dict
 * will do the trick.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "forth.lex.h"		/* #defines for lexical analysis */

#define isoctal(c)	(c >= '0' && c <= '7')	/* augument ctype.h */

#define assert(c,s)	(!(c) ? failassert(s) : 1)
#define chklit()	ChkLit(token,prev_lit)

#define LINK struct linkrec
#define CHAIN struct chainrec

struct chainrec {
    char chaintext[32];
    Cell defloc;			/* CFA or label loc */
    int chaintype;			/* 0=undef'd, 1=absolute, 2=relative */
    CHAIN *nextchain;
    LINK *firstlink;
};

struct linkrec {
    Cell loc;
    LINK *nextlink;
};

CHAIN firstchain;

#define newchain()	(CHAIN *)(calloc(1,sizeof(CHAIN)))
#define newlink()	(LINK *)(calloc(1,sizeof(LINK)))

CHAIN *find();
CHAIN *lastchain();
LINK *lastlink();

Cell dp = DPBASE;
Cell latest;

Cell mem[INITMEM];

FILE *outf;

void builddict();
void mkrest();
void buildcore();
void checkdict();
void writedict();
Cell instance(char*);
void dicterr(char*,...);
void mkword(char *s,Word);
Cell mkval(TOKEN*);
void comma(Word);
void mkstr(char*);
void skipcomment();
void define(char*,Word);
void dictwarn(char*);
Cell mkdecimal(char*);
Cell mkhex(char*);
Cell mkoctal(char*);
void printword(Word);

void ChkLit(token, prev_lit)
TOKEN *token;
Cell prev_lit;
{
        if (!prev_lit) {
                dictwarn("Questionable literal");
                fprintf(stderr,"ChkLit: %s\n", token->text);
        }
}

int main(argc, argv)
int argc;
char *argv[];
{
#ifdef DEBUG
	puts("Opening output file");
#endif /* DEBUG */

    strcpy(firstchain.chaintext," ** HEADER **");
    firstchain.nextchain = NULL;
    firstchain.firstlink = NULL;

#ifdef DEBUG
    puts("call builddict");
#endif /* DEBUG */
    builddict();
#ifdef DEBUG
    puts("Make FORTH and COLDIP");
#endif /* DEBUG */
    mkrest();
#ifdef DEBUG
    puts("Call Buildcore");
#endif /* DEBUG */
    buildcore();
#ifdef DEBUG
    puts("call checkdict");
#endif /* DEBUG */
    checkdict();
#ifdef DEBUG
    puts("call writedict");
#endif /* DEBUG */
    writedict();

    printf("%s: done.\n", argv[0]);
}

void buildcore()			/* set up low core */
{
	mem[USER_DEFAULTS+0] = INITS0;			/* initial S0 */
	mem[USER_DEFAULTS+1] = INITR0;			/* initial R0 */
	mem[USER_DEFAULTS+2] = TIB_START;		/* initial TIB */
	mem[USER_DEFAULTS+3] = MAXWIDTH;		/* initial WIDTH */
	mem[USER_DEFAULTS+4] = 0;			/* initial WARNING */
	mem[USER_DEFAULTS+5] = dp;			/* initial FENCE */
	mem[USER_DEFAULTS+6] = dp;			/* initial DP */
	mem[USER_DEFAULTS+7] = instance("FORTH") + 3;	/* initial CONTEXT */

	mem[SAVEDIP] = 0;				/* not a saved FORTH */
}

void builddict()			/* read the dictionary */
{
    int prev_lit = 0, lit_flag = 0;
    Cell temp;
    char s[256];
    TOKEN *token;

    while ((token = yylex()) != NULL) {	/* EOF returned as a null pointer */
#ifdef DEBUG
	printf("\ntoken: %s: %d ",token->text, token->type);
#endif /* DEBUG */
	switch (token->type) {

	case PRIM:
#ifdef DEBUG
	    printf("primitive ");
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)	/* get the next word */
		dicterr("No word following PRIM");
	    strcpy (s,token->text);
#ifdef DEBUG
	    printf(".%s. ",s);
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)	/* get the value */
		dicterr("No value following PRIM <word>");
	    mkword(s,(Cell)mkval(token));
	    break;

	case CONST:
#ifdef DEBUG
	    printf("constant ");
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)	/* get the word */
		dicterr("No word following CONST");
	    strcpy (s,token->text);		/* s holds word */
#ifdef DEBUG
	    printf(".%s. ",s);
#endif /* DEBUG */
	    if (!find("DOCON"))
		dicterr ("Constant definition before DOCON: %s",s);
				/* put the CF of DOCON into this word's CF */
	    mkword(s,(Cell)mem[instance("DOCON")]);
	    if ((token = yylex()) == NULL)	/* get the value */
		dicterr("No value following CONST <word>");
	    temp = mkval(token);

	    /* two special-case constants */
	    if (strcmp(s,"FIRST") == 0) temp = INITR0;
	    else if (strcmp(s,"LIMIT") == 0) temp = DPBASE;

	    comma(temp);
	    break;

	case VAR:
#ifdef DEBUG
	    printf("variable ");
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)	/* get the variable name */
		dicterr("No word following VAR");
	    strcpy (s,token->text);
#ifdef DEBUG
	    printf(".%s. ",s);
#endif /* DEBUG */
	    if (!find("DOVAR"))
		dicterr("Variable declaration before DOVAR: %s",s);
	    mkword (s, (Cell)mem[instance("DOVAR")]);
	    if ((token = yylex()) == NULL)	/* get the value */
		dicterr("No value following VAR <word>");
	    comma(mkval(token));
	    break;

	case USER:
#ifdef DEBUG
	    printf("uservar ");
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)	/* get uservar name */
		dicterr("No name following USER");
	    strcpy (s,token->text);
#ifdef DEBUG
	    printf(".%s. ",s);
#endif /* DEBUG */
	    if (!find("DOUSE"))
		dicterr("User variable declared before DOUSE: %s",s);
	    mkword (s, (Cell)mem[instance("DOUSE")]);
	    if ((token = yylex()) == NULL)	/* get the value */
		dicterr("No value following USER <word>");
	    comma(mkval(token));
	    break;

	case COLON:
#ifdef DEBUG
	    printf("colon def'n ");
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)	/* get name of word */
		dicterr("No word following : in definition");
	    strcpy (s,token->text);
#ifdef DEBUG
	    printf(".%s.\n",s);
#endif /* DEBUG */
	    if (!find("DOCOL"))
		dicterr("Colon definition appears before DOCOL: %s",s);

	    if (token->type == NUL) {	/* special zero-named word */
		Cell here = dp;		/* new latest */
#ifdef DEBUG
		printf("NULL WORD AT 0x%04x\n");
#endif /* DEBUG */
		comma(0xC1);
		comma(0x80);
		comma(latest);
		latest = here;
		comma((Cell)mem[instance("DOCOL")]);
	    }
	    else {
		mkword (s, (Cell)mem[instance("DOCOL")]);
	    }
	    break;

	case SEMICOLON:
#ifdef DEBUG
	    puts("end colon def'n");
#endif /* DEBUG */
	    comma (instance(";S"));
	    break;

	case SEMISTAR:
#ifdef DEBUG
	    printf("end colon w/IMMEDIATE ");
#endif /* DEBUG */
	    comma (instance (";S"));	/* compile cfA of ;S, not CF */
	    mem[latest] |= IMMEDIATE;	/* make the word immediate */
	    break;

	case STRING_LIT:
#ifdef DEBUG
	    printf("string literal ");
#endif /* DEBUG */
	    strcpy(s,token->text);
	    mkstr(s);		/* mkstr compacts the string in place */
#ifdef DEBUG
	    printf("string=(%d) \"%s\" ",strlen(s),s);
#endif /* DEBUG */
	    comma(strlen(s));
	    {
		char *stemp;
		stemp = s;
		while (*stemp) comma(*stemp++);
	    }
	    break;
	
	case COMMENT:
#ifdef DEBUG
	    printf("comment ");
#endif /* DEBUG */
	    skipcomment();
	    break;

	case LABEL:
#ifdef DEBUG
	    printf("label: ");
#endif /* DEBUG */
	    if ((token = yylex()) == NULL)
		dicterr("No name following LABEL");
#ifdef DEBUG
	    printf(".%s. ", token->text);
#endif /* DEBUG */
	    define(token->text,2);	/* place in sym. table w/o compiling
					   anything into dictionary; 2 means
					   defining a label */
	    break;

	case LIT:
		lit_flag = 1;		/* and fall through to the rest */

	default:
	    if (find(token->text) != NULL) {	/* is word defined? */
#ifdef DEBUG
		printf("  normal: %s\n",token->text);
#endif /* DEBUG */
	    	comma (instance (token->text));
		break;
	    }

	    /* else */
	    /* the literal types all call chklit(). This macro checks to
	       if the previous word was "LIT"; if not, it warns */
	    switch(token->type) {
	    case DECIMAL: chklit(); comma(mkdecimal(token->text)); break;
	    case HEX: chklit(); comma(mkhex(token->text)); break;
	    case OCTAL: chklit(); comma(mkoctal(token->text)); break;
	    case C_BS: chklit(); comma('\b'); break;
	    case C_FF: chklit(); comma('\f'); break;
	    case C_NL: chklit(); comma('\n'); break;
	    case C_CR: chklit(); comma('\r'); break;
	    case C_TAB: chklit(); comma('\t'); break;
	    case C_BSLASH: chklit(); comma(0x5c); break;  /* ASCII backslash */
	    case C_LIT: chklit(); comma(*((token->text)+1)); break;

	    default:
#ifdef DEBUG
		printf("forward reference");
#endif /* DEBUG */
		comma (instance (token->text));		/* create an instance,
						to be resolved at definition */
	    }
	}
#ifdef DEBUG
	if (lit_flag) puts("expect a literal");
#endif /* DEBUG */
	prev_lit = lit_flag;	/* to be used by chklit() next time */
	lit_flag = 0;
    }
}

void comma(i)		        /* put at mem[dp]; increment dp */
Word i;
{
    mem[dp++] = (UCell)i;
    if (dp > INITMEM) dicterr("DICTIONARY OVERFLOW");
}

/*
 * make a word in the dictionary.  the new word will have name *s, its CF
 * will contain v. Also, resolve any previously-unresolved references by
 * calling define()
 */

void mkword(s, v)
char *s;
Word v;
{
	Cell here, count = 0;
	char *olds;
	olds = s;		/* preserve this for resolving references */

#ifdef DEBUG
	printf("%s ",s);
#endif /* DEBUG */

	here = dp;		/* hold this value to place length byte */

	while (*s) {		/* for each character */
		mem[++dp] = (UCell)*s;
		count++; s++;
	}

	if (count >= MAXWIDTH) dicterr("Input word name too long");

				/* set MSB on */
	mem[here] = (Cell)(count | 0x80);

	mem[dp++] |= 0x80;	/* set hi bit of last char in name */
	
	mem[dp++] = (Cell)latest;	/* the link field */

	latest = here;		/* update the link */

	mem[dp] = (Cell)v;	/* code field; leave dp = CFA */

	define(olds,1);		/* place in symbol table. 1 == "not a label" */
	dp++;			/* now leave dp holding PFA */

	/* that's all. Now dp points (once again) to the first UNallocated
           spot in mem, and everybody's happy. */
}

void mkrest()			/* Write out the word FORTH as a no-op with
				   DOCOL as CF, ;S as PF, followed by
				   0xA081, and latest in its PF.
				   Also, Put the CFA of ABORT at 
				   mem[COLDIP] */
{
	int temp;

	mem[COLDIP] = dp;	/* the cold-start IP is here, and the word
				   which will be executed is COLD */
	if ((mem[dp++] = instance("COLD")) == 0)
		dicterr("COLD must be defined to take control at startup");

	mem[ABORTIP] = dp;	/* the abort-start IP is here, and the word
				   which will be executed is ABORT */
	if ((mem[dp++] = instance("ABORT")) == 0)
		dicterr("ABORT must be defined to take control at interrupt");

	mkword("FORTH",mem[instance("DOCOL")]);
	comma(instance(";S"));
	comma(0xA081);	/* magic number for vocabularies */
	comma(latest);		/* NFA of last word in dictionary: FORTH */

	mem[LIMIT] = dp + 1024;
	if (mem[LIMIT] >= INITMEM) mem[LIMIT] = INITMEM-1;
}

void writedict()		/* write memory to COREFILE and map 
			   	   to MAPFILE */
{
    FILE   *outfile;
    int     i, temp, tempb, firstzero, nonzero;
    char    chars[9], outline[80], tstr[6];

    chars[8] = '\0';
    outfile = fopen(MAPFILE,"w");

    for (temp = 0; temp < dp; temp += 8) {
	nonzero = FALSE;
	sprintf (outline, FMT_HEXCELL, temp);
        strcat (outline, ":");
	for (i = temp; i < temp + 8; i++) {
            strcat (outline, " ");
	    sprintf (tstr, FMT_HEXCELL, (UCell) mem[i]);
	    strcat (outline, tstr);
	    tempb = mem[i] & 0x7f;
	    if (tempb < 0x7f && tempb >= ' ')
		chars[i % 8] = tempb;
	    else
		chars[i % 8] = '.';
	    nonzero |= mem[i];
	}
	if (nonzero) {
	    fprintf (outfile, "%s %s\n", outline, chars);
	    firstzero = TRUE;
	}
	else
	    if (firstzero) {
		fprintf (outfile, "----- ZERO ----\n");
		firstzero = FALSE;
	    }
    }
    fclose (outfile);


    printf ("Writing %s; DPBASE=%d; dp=%d\n", COREFILE, DPBASE, dp);

    if ((outf = fopen (COREFILE, "w")) == NULL) {
	printf ("nf: can't open %s for output.\n", COREFILE);
	exit (1);
    }

    if (fwrite (mem, sizeof (*mem), mem[LIMIT], outf) != mem[LIMIT]) {
	fprintf (stderr, "Error writing to %s\n", COREFILE);
	exit (1);
    }

    if (fclose (outf) == EOF) {
	fprintf (stderr, "Error closing %s\n", COREFILE);
	exit (1);
    }
}

Cell mkval(t)			/* convert t->text to integer based on type */
TOKEN *t;
{
	char *s = t->text;
	int sign = 1;

	if (*s == '-') {
		sign = -1;
		s++;
	}

	switch (t->type) {
	case DECIMAL:
		return (sign * mkdecimal(s));
	case HEX:
		return (sign * mkhex(s));
	case OCTAL:
		return (sign * mkoctal(s));
	default:
		dicterr("Bad value following PRIM, CONST, VAR, or USER");
	}
        return 0;
}

Cell mkhex(s)
char *s;
{				/*  convert hex ascii to integer */
    Cell    temp;
    temp = 0;

    s += 2;			/* skip over '0x' */
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

Cell mkoctal(s)
char *s;
{				/*  convert Octal ascii to integer */
    Cell   temp;
    temp = 0;

    while (isoctal (*s)) {	/* first non-octal char ends */
	temp = temp * 8 + (*s - '0');
	s++;
    }
    return temp;
}

Cell mkdecimal(s)		/* convert ascii to decimal */
char *s;
{
    Cell    temp;
    temp = 0;

    while (isdigit (*s)) {
        temp = temp * 10 + (*s - '0');
        s++;
    }
    return temp;
}

void dicterr(char *s,...)
{
    va_list ap;
    va_start(ap,s);
    vfprintf(stderr,s,ap);
    va_end(ap);
    fflush(stderr);

    fprintf(stderr,"\nLast word defined was ");
    printword(latest);
/*    fprintf(stderr, "; last word read was \"%s\"", token->text); */
    fprintf(stderr,"\n");
    exit(1);
}

void dictwarn(s)	/* almost like dicterr, but don't exit */
char *s;
{
    fprintf(stderr,"\nWarning: %s\nLast word read was ",s);
    printword(latest);
    putc('\n',stderr);
}
    
void printword(n)
Word n;
{
    int count, tmp;
    count = mem[n] & 0x1f;
    for (n++;count;count--,n++) {
	tmp = mem[n] & ~0x80;		/* mask eighth bit off */
	if (tmp >= ' ' && tmp <= '~') putc(tmp, stderr);
    }
}

void skipcomment()
{
    while(getchar() != ')');
}

void mkstr(s)			/* modifies a string in place with escapes
				   compacted. Strips leading & trailing \" */
char *s;
{
    char *source;
    char *dest;

    source = dest = s;
    source++;			/* skip leading quote */
    while (*source != '"') {	/* string ends with unescaped \" */
	if (*source == '\\') {	/* literal next */
	    source++;
	}
	*dest++ = *source++;
    }
    *dest = '\0';
}

void failassert(s)
char *s;
{
    puts(s);
    exit(1);
}

void checkdict()			/* check for unresolved references */
{
    CHAIN *ch = &firstchain;

#ifdef DEBUG
    puts("\nCheck for unresolved references");
#endif /* DEBUG */
    while (ch != NULL) {
#ifdef DEBUG
	printf("ch->chaintext = .%s. - ",ch->chaintext);
#endif /* DEBUG */
	if ((ch->firstlink) != NULL) {
	    fprintf(stderr,"Unresolved forward reference: %s\n",ch->chaintext);
#ifdef DEBUG
	    puts("still outstanding");
#endif /* DEBUG */
	}
#ifdef DEBUG
	else puts("clean.");
#endif /* DEBUG */
	ch = ch->nextchain;
    }
}

    
/********* structure-handling functions find(s), define(s,t), instance(s) **/

CHAIN *find(s)		/* returns a pointer to the chain named s */
char *s;
{
	CHAIN *ch;
	ch = &firstchain;
	while (ch != NULL) {
		if (strcmp (s, ch->chaintext) == 0) return ch;
		else ch = ch->nextchain;
	}
	return NULL;	/* not found */
}

/* define must create a symbol table entry if none exists, with type t.
   if one does exist, it must have type 0 -- it is an error to redefine
   something at this stage. Change to type t, and fill in the outstanding
   instances, with the current dp if type=1, or relative if type=2. */

void define(s,t)	/* define s at current dp */
char *s;
Word t;
{
	CHAIN *ch;
	LINK *ln, *templn;

#ifdef DEBUG
	printf("define(%s,%d)\n",s,t);
#endif /* DEBUG */

	if (t < 1 || t > 2)	/* range check */
		dicterr("Program error: type in define() not 1 or 2.");

	if ((ch = find(s)) != NULL) {		/* defined or instanced? */
		if (ch -> chaintype != 0)	/* already defined! */
			dicterr("Word already defined: %s",s);
		else {
#ifdef DEBUG
			printf("there are forward refs: ");
#endif /* DEBUG */
			ch->chaintype = t;
			ch->defloc = dp;
		}
	}
	else {				/* must create a (blank) chain */
#ifdef DEBUG
		puts("no forward refs");
#endif /* DEBUG */
		/* create a new chain, link it in, leave ch pointing to it */
		ch = ((lastchain() -> nextchain) = newchain());
		strcpy(ch->chaintext, s);
		ch->chaintype = t;
		ch->defloc = dp;	/* fill in for future references */
	}

	/* now ch points to the chain (possibly) containing forward refs */
	if ((ln = ch->firstlink) == NULL) return;	/* no links! */

	while (ln != NULL) {
#ifdef DEBUG
		printf("    Forward ref at 0x%x\n",ln->loc);
#endif /* DEBUG */
		switch (ch->chaintype) {
		case 1: mem[ln->loc] = (Cell)dp;	/* absolute */
			break;
		case 2: mem[ln->loc] = (Cell)(dp - ln->loc);	/* relative */
			break;
		default: dicterr ("Bad type field in define()");
		}

		/* now skip to the next link & free this one */
		templn = ln;
		ln = ln->nextlink;
		free(templn);
	}
	ch->firstlink = NULL;	/* clean up that last pointer */
}

/*
   instance must return a value to be compiled into the dictionary at
   dp, consistent with the symbol s: if s is undefined, it returns 0,
   and adds this dp to the chain for s (creating that chain if necessary).
   If s IS defined, it returns <s> (absolute) or (s-dp) (relative), 
   where <s> was the dp when s was defined.
*/

Cell instance(s)
char *s;
{
	CHAIN *ch;
	LINK *ln;

#ifdef DEBUG
	printf("instance(%s):\n",s);
#endif /* DEBUG */

	if ((ch = find(s)) == NULL) {	/* not defined yet at all */
#ifdef DEBUG
		puts("entirely new -- create a new chain");
#endif /* DEBUG */
		/* create a new chain, link it in, leave ch pointing to it */
		ch = ((lastchain() -> nextchain) = newchain());

		strcpy(ch->chaintext, s);
		ln = newlink();		/* make its link */
		ch->firstlink = ln;
		ln->loc = dp;		/* store this location there */
		return 0;		/* all done */
	}
	else {
		switch(ch->chaintype) {
		case 0:			/* not defined yet */
#ifdef DEBUG
			puts("still undefined -- add a link");
#endif /* DEBUG */
			/* create a new link, point the last link to it, and
			   fill in the loc field with the current dp */
			(lastlink(ch)->nextlink = newlink()) -> loc = dp;
			return 0;
		case 1:			/* absolute */
#ifdef DEBUG
			puts("defined absolute.");
#endif /* DEBUG */
			return ch->defloc;
		case 2:			/* relative */
#ifdef DEBUG
			puts("defined relative.");
#endif /* DEBUG */
			return ch->defloc - dp;
		default:
			dicterr("Program error: bad type for chain");
		}
	}
        return 0;
}

CHAIN *lastchain()	/* starting from firstchain, find the last chain */
{
	CHAIN *ch = &firstchain;
	while (ch->nextchain != NULL) ch = ch->nextchain;
	return ch;
}

LINK *lastlink(ch)	/* return the last link in the chain */
CHAIN *ch;		/* CHAIN MUST HAVE AT LEAST ONE LINK */
{
	LINK *ln = ch->firstlink;

	while (ln->nextlink != NULL) ln = ln->nextlink;
	return ln;
}

int yywrap()	/* called by yylex(). returning 1 means "all finished" */
{
    return 1;
}
