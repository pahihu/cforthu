/*
 * prims.c -- code for the primitive functions declared in forth.dict
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>	/* used in "digit" */
#include "common.h"
#include "forth.h"
#include "prims.h"	/* macro primitives */

/*
             ----------------------------------------------------
                            PRIMITIVE DEFINITIONS
             ----------------------------------------------------
*/

void zbranch()			/* add an offset (branch) if tos == 0 */
{
	if(pop() == 0) 
	    ip += mem[ip];
	else
	    ip++;		/* else skip over the offset */
}

void ploop()			/* (loop) -- loop control */
{
	Cell index, limit;
	index = rpop()+1;
	if(index < (limit = rpop())) {   /* if the new index < the limit */
		rpush(limit);	/* restore the limit */
		rpush(index);	/* and the index (incremented) */
		branch();	/* and go back to the top of the loop */
	}
	else ip++;     		/* skip over the offset, and exit, having
				   popped the limit & index */
}

void pploop()			/* (+loop) -- almost the same */
{
	Cell index, limit;
	index = rpop()+pop();		/* get index & add increment */
	if(index < (limit = rpop())) {	/* if new index < limit */
		rpush (limit);		/* restore the limit */
		rpush (index);		/* restore the new index */
		branch();		/* and branch back to the top */
	}
	else {
		ip++;		/* skip over branch offset */
	}
}

void pdo()		/* (do): limit init -- [pushed to rstack] */
{
    swap();
    rpush (pop());
    rpush (pop());
}

void i()		/* copy top of return stack to cstack */
{
    int tmp;
    tmp = rpop();
    rpush(tmp);
    push(tmp);
}

void r()	/* this must be a primitive as well as I because otherwise it
		   always returns its own address */
{
    i();
}

void digit()		/* digit: c -- FALSE or [v TRUE] */
{
    Cell c, base;		/* C is ASCII char, convert to val. BASE is
				   used for range checking */
    base = pop();
    c = pop();
    if (!isascii(c)) {
	push (FALSE);
	return;
    }
 				/* lc -> UC if necessary */
    if (islower(c)) c = toupper(c);

    if (c < '0' || (c > '9' && c < 'A') || c > 'Z') {
	push(FALSE);		/* not a digit */
    }
    else {			/* it is numeric or UC Alpha */
	if (c >= 'A') c -= 7;	/* put A-Z right after 0-9 */

	c -= '0';		/* now c is 0..35 */

	if (c >= base) {
	    push (FALSE);	/* FALSE - not a digit */
	}
	else {			/* OKAY: push value, then TRUE */
	    push (c);
	    push (TRUE);
	}
    }
}

void pfind()	/* WORD TOP -- xx FLAG, where TOP is NFA to start at;
		   WORD is the word to find; xx is PFA of found word;
		   yy is actual length of the word found;
		   FLAG is 1 if found. If not found, 0 alone is stacked. */
{
    UCell  worka, workb, workc, current, word, match;
    unsigned char cha, chb;

    current = pop ();
    word = pop ();
    while (current) {		/* stop at end of dictionary */
	if (!((mem[current] ^ mem[word]) & 0x3f)) {
				/* match lengths & smudge */
	    worka = current + 1;/* point to the first letter */
	    workb = word + 1;
	    workc = mem[word];	/* workc gets count */
	    match = TRUE;	/* initally true, for looping */
	    while (workc-- && match) {
                cha = mem[worka++] & 0x7F;
                chb = mem[workb++] & 0x7F;
                match = cha == chb;
                if (!match && isalpha(cha))
                    match = tolower(cha) == tolower(chb);
		/* match = ((mem[worka++] & 0x7f) == (mem[workb++] & 0x7f)); */
            }
	    if (match) {	/* exited with match TRUE -- FOUND IT */
		push (worka + 2);		/* worka=LFA; push PFA */
		push (mem[current]);		/* push length byte */
		push (TRUE);			/* and TRUE flag */
		return;
	    }
	}
	/* failed to match */
	/* follow link field to next word */
	current = mem[current + (mem[current] & 0x1f) + 1];
    }
    push (FALSE);		/* current = 0; end of dict; not found */
}

void enclose()
{
	int delim, current, offset;

	delim = pop();
	current = pop();
	push (current);

	offset = -1;
	current--;
encl1:
	current++;
	offset++;
	if (mem[current] == delim) goto encl1;

	push(offset);
	if (mem[current] == 0) {
		offset++;
		push (offset);
		offset--;
		push (offset);
		return;
	}

encl2:
	current++;
	offset++;
	if (mem[current] == delim) goto encl4;
	if (mem[current] != 0) goto encl2;

	/* mem[current] is null.. */
	push (offset);
	push (offset);
	return;

encl4:	/* found the trailing delimiter */
	push (offset);
	offset++;
	push (offset);
	return;
}

void cmove()		/* cmove: source dest number -- */
{
    Cell source, dest, number, i;
    number = pop();
    dest = pop();
    source = pop();
    for ( ; number ; number-- ) mem[dest++] = mem[source++];
}

void fill()		/* fill: c dest number -- */
{
    Cell dest, number, c;
    number = pop();
    dest = pop();
    c = pop();

    mem[dest] = c;		/* always at least one */
    if (number == 1) return;	/* return if only one */

    push (dest);		/* else push dest as source of cmove */
    push (dest + 1);		/* dest+1 as dest of cmove */
    push (number - 1);		/* number-1 as number of cmove */
    cmove();
}

void ustar()			/* u*: a b -- a*b.hi a*b.lo */
{
    UCell a, b;
    UDCell c;
    a = (UCell)pop();
    b = (UCell)pop();
    c = a * b;

    /* (Cell) -1 is probably FFFF..., which is just what we want */
    push ((UCell)(c & (Cell) -1));	      /* low word of product */
					      /* high word of product */
    push (DCHIGH(c));
}

void uslash()		/* u/: NUM.LO NUM.HI DENOM -- REM QUOT */
{
    UCell numhi, numlo, denom;
    UCell quot, remainder;	        /* the DCell below are to be sure the
					   intermediate computation is done
					   DCell; the results are Cell */
    denom = pop();
    numhi = pop();
    numlo = pop();
    if (denom == 0) {
        quot = (UCell) -1;
        remainder = (UCell) -1;
    }
    else {
        quot = ((UDCell)MKDCELL(numhi,numlo)) / (UDCell)denom;
        remainder = ((UDCell)MKDCELL(numhi,numlo)) % (UDCell)denom;
    }

    push (remainder);
    push (quot);
}

void swap()			/* swap: a b -- b a */
{
    Cell a, b;
    b = pop();
    a = pop();
    push (b);
    push (a);
}

void rot()			/* rotate */
{
    Cell a, b, c;
    a = pop ();
    b = pop ();
    c = pop ();
    push (b);
    push (a);
    push (c);
}

void tfetch()			/* 2@: addr -- mem[addr+1] mem[addr] */
{
    UCell addr;
    addr = pop();
    push (mem[addr + 1]);
    push (mem[addr]);
}

void store()		        /* !: val addr -- <set mem[addr] = val> */
{
    UCell tmp;
    tmp = pop();
    mem[tmp] = pop();
}

void cstore()		        /* C!: val addr --  */
{
#if 0
    UCell tmp;
    tmp = pop();
    bytmem[tmp] = pop();
#endif
    store();
}

void tstore()			/* 2!: val1 val2 addr -- 
				   mem[addr] = val2,
				   mem[addr+1] = val1 */
{
    UCell tmp;
    tmp = pop();
    mem[tmp] = pop();
    mem[tmp+1] = pop();
}

void leave()			/* set the index = the limit of a DO */
{
    int tmp;
    rpop();			/* discard old index */
    tmp = rpop();		/* and push the limit as */
    rpush(tmp);			/* both the limit */
    rpush(tmp);			/* and the index */
}

void dplus()			/* D+: double-add */
{
    Cell ahi, alo, bhi, blo;
    DCell a, b;
    bhi = pop();
    blo = pop();
    ahi = pop();
    alo = pop();
    a = MKDCELL(ahi,alo);
    b = MKDCELL(bhi,blo);
    a = a + b;
    push ((UCell)(a & (Cell) -1));	        /* sum lo */
    push (DCHIGH(a));                           /* sum hi */
}

void subtract()			/* -: a b -- (a-b) */
{
    Cell tmp;
    tmp = pop();
    push (pop() - tmp);
}

void dsubtract()		/* D-: double-subtract */
{
    Cell ahi, alo, bhi, blo;
    DCell a, b;
    bhi = pop();
    blo = pop();
    ahi = pop();
    alo = pop();
    a = MKDCELL(ahi,alo);
    b = MKDCELL(bhi,blo);
    a = a - b;
    push ((UCell)(a & (Cell) -1));	        /* diff lo */
    push (DCHIGH(a));                           /* diff hi */
}

void dminus()			/* DMINUS: negate a double number */
{
    UCell ahi, alo;
    DCell a;
    ahi = pop();
    alo = pop();
    a = -MKDCELL(ahi,alo);
    push ((UCell)(a & (Cell) -1));		/* -a lo */
    push ((UCell)DCHIGH(a));                    /* -a hi */
}

void over()			/* over: a b -- a b a */
{
    Cell a, b;
    b = pop();
    a = pop();
    push (a);
    push (b);
    push (a);
}

void dup()			/* dup: a -- a a */
{
    Cell a;
    a = pop();
    push (a);
    push (a);
}

void tdup()		        /* 2dup: a b -- a b a b */
{
    Cell a, b;
    b = pop();
    a = pop();
    push (a);
    push (b);
    push (a);
    push (b);
}

void pstore()			/* +!: val addr -- <add val to mem[addr]> */
{
    Cell addr, val;
    addr = pop();
    val = pop();
    mem[addr] += val;
}

void toggle()			/* toggle: addr bits -- <xor mem[addr]
				   with bits, store in mem[addr]> */
{
    Cell bits, addr;
    bits = pop();
    addr = pop();
    mem[addr] ^= bits;
}

void less()
{
    int tmp;
    tmp = pop();
    push (pop() < tmp);
}

void pcold()
{
    csp = INITS0;		/* initialize values */
    rsp = INITR0;
	/* copy USER_DEFAULTS area into UP area */
    push (USER_DEFAULTS);	/* source */
    push (UP);			/* dest */
    push (DEFS_SIZE);		/* count */
    cmove();			/* move! */
				/* returns, executes ABORT */
}

void prslw()
{
	int buffer, flag, addr, i, temp, unwrittenflag = FALSE;
	long fpos;
	char buf[1024];		/* holds data for xfer */

	flag = pop();
	buffer = pop();
	addr = pop();
	fpos = (long) (buffer * 1024);

					/* extend if necessary */
	if (fpos >= bfilesize) {
	    if (flag == 0) { 		/* write */
		printf("Extending block file to %lD bytes\n", fpos+1024);
		/* the "2" below is the fseek magic number for "beyond end" */
		fseek(blockfile, (fpos+1024) - bfilesize, SEEK_END);
		bfilesize = ftell(blockfile);
	    }
	    else {			/* reading unwritten data */
		unwrittenflag = TRUE;	/* will read all zeroes */
	    }
	}
	else {
		/* note that "0" below is fseek magic number for "relative to
		   beginning-of-file" */
		fseek(blockfile, fpos, SEEK_SET); /* seek to destination */
	}

	if (flag) {		/* read */
	    if (unwrittenflag) {	/* not written yet */
		for (i=0; i<1024; i++) mem[addr++] = 0;	/* "read" nulls */
	    }
	    else {			/* does exist */
		if ((temp = fread (buf, sizeof(char), 1024, blockfile)) 
								!= 1024) {
			fprintf (stderr,
				"File read error %d reading buffer %d\n",
					temp, buffer);
			errexit("");
		}
		for (i=0; i<1024; i++) mem[addr++] = buf[i];
	    }
	}
	else {	/* write */
		for (i=0; i<1024; i++) buf[i] = mem[addr++];
		if ((temp = fwrite (buf, sizeof(char), 1024, blockfile))
								 != 1024) {
			    fprintf(stderr,
				"File write error %d writing buffer %d\n",
					temp, buffer);
			    errexit("");
		}
	}
}

void psave()
{
	FILE *fp;

	printf("\nSaving...");
	fflush(stdout);
	mem[SAVEDIP] = ip;	/* save state */
	mem[SAVEDSP] = csp;
	mem[SAVEDRP] = rsp;

	if ((fp = fopen(sfilename,"w")) == NULL)  /* open for writing only */
		errexit("Can't open core file %s for writing\n", sfilename);
	if (fwrite(mem, sizeof(*mem), mem[0], fp) != mem[0])
		errexit("Write error on %s\n",sfilename);
	if (fclose(fp) == EOF)
		errexit("Close error on %s\n",sfilename);
	puts("Saved. Exit FORTH.");
	exit(0);
}

void plimit()
{
        push ((bfilesize / 1024) - 1);
}

/* vim: set ts=8 sw=8: */
