/* prims.h: This file defines inline primitives, which are called as functions
   from the big SWITCH in forth.c */

 				/* push mem[ip] to cstack */
#define lit() { push (mem[ip++]); }
			/* add an offset (this word) to ip */
#define branch() { ip += mem[ip]; }
			/* return a key from input */
#define key() { push(pkey()); }
		/* return TRUE if break key pressed */
#define qterminal() { pqterm(); }
				/* and: a b -- a & b */
#define and() { push (pop() & pop()); }
				/* or: a b -- a | b */
#define or() { push (pop() | pop()); }
				/* xor: a b -- a ^ b */
#define xor() { push (pop() ^ pop()); }
			/* sp@: push the stack pointer */
#define spfetch() { push (csp); }
			/* sp!: load initial value into SP */
#define spstore() { csp = mem[S0]; }
			/* rp@: fetch the return stack pointer */
#define rpfetch() { push (rsp); }
			/* rp!: load initial value into RP */
#define rpstore() { rsp = mem[R0]; }
			/* ;S: ends a colon definition. */
#define semis() { ip = rpop(); }
			/* @: addr -- mem[addr] */
#define fetch() { push (mem[pop()]); }
			/* C@: addr -- mem[addr] */
#define cfetch() { push (mem[pop()] & 0xff); }
			/* push to return stack */
#define tor() { rpush(pop()); }
			/* pop from return stack */
#define fromr() { push (rpop()); }
			/* 0=: a -- (a == 0) */
#define zeq() { push ( pop() == 0 ); }
			/* 0<: a -- (a < 0) */
#define zless() { push ( pop() < 0 ); }
			/* +: a b -- (a+b) */
#define plus() { push (pop () + pop ()); }
			/* MINUS: negate a number */
#define minus() { push (-pop()); }
				/* drop: a -- */
#define drop() { pop(); }
			/* DOCOL: push ip & start a thread */
#define docol() { rpush(ip); ip = w+1; }
			/* do a constant: push the value at mem[w+1] */
#define docon() { push (mem[w+1]); }
			/* do a variable: push (w+1) (the PFA) to the stack */
#define dovar() { push (w+1); }
		/* execute a user variable: add UP to the offset found in PF */
#define douse() { push (mem[w+1] + ORIGIN); }

#define allot() { Callot (pop()); }
				/* comparison tests */
#define equal() { push(pop() == pop()); }
				/* not equal */
#define noteq() { push (pop() != pop()); }
				/* DODOES */
#define dodoes() { rpush(ip); ip = mem[w+1]; push (w+2); }
				/* DOVOC -- not supported */
#define dovoc() { errexit("VOCABULARIES are not supported."); }
				/* (BYE) -- exit with error code */
#define pbye() { exit(0); }
				/* TRON -- trace at pop() depth */
#define tron() { trace = TRUE; tracedepth = pop(); }
				/* TROFF -- stop tracing */
#define troff() { trace = 0; }

/* function declarations */
void zbranch();
void ploop();
void pploop();
void pdo();
void i();
void r();
void digit();
void pfind();
void enclose();
void cmove();
void fill();
void ustar();
void uslash();
void swap();
void rot();
void tfetch();
void store();
void cstore();
void tstore();
void leave();
void dplus();
void subtract();
void dsubtract();
void dminus();
void over();
void dup();
void tdup();
void pstore();
void toggle();
void less();
void pcold();
void prslw();
void psave();
