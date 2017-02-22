Category 1,  Topic 22
Message 4         Mon Oct 09, 1989
GARY-S                       at 14:01 EDT
 
 
 From: marc@noe.UUCP (Marc de Groot)

 Subject: [4mRe: Alan Pratt's C-Forth
 Date: 8 Oct 89 01:52:05 GMT

 In article <4825@cps3xx.UUCP> jhl@frith.egr.msu.edu () writes:
 >I am using Alan Pratt's C-Forth under SCO XENIX 386 2.3.1 and have the
 >following problem:
 >
 >In the editor, when I edit an existing block, mark it for updating,
 >and flush it out to disk, exit forth, restart forth and look at the
 >block file, the original block is unchanged and the block which results
 >from my modifications appears at the end of the block file. 

 C-Forth contains significant errors, especially in the disk I/O code.
 Try the following fixes.  I have not used the interpreter extensively
 but these fixes seem to work on preliminary testing.

 The following piece of code contains a fix for the problem you
 mentioned.  See the first comment below.  This code fragment
 replaces getblockfile() in forth.c

 getblockfile()
 {
       /* M000 Changed the file mode from "a+" to "r+" on next line
        * and added the call to fseek. Also added the declaration
        * of ftell() and fopen().
        */
       long ftell(); /* M000 */
       FILE *fopen(); /* M000 */

       if ((blockfile = fopen(bfilename, "r+")) == NULL) /* M000 */
               errexit("Can't open blockfile \"%s\"\n", bfilename);
       fseek(blockfile, 0L, 2); /* M000 */
       bfilesize = ftell(blockfile);

       printf("Block file has %d blocks.\n",(int) (bfilesize/1024) - 1);
 }

 In prims.c replace prslw() with the following code.  SLOWSTACK should not
 be defined.  Cell should be typedef'd to int (or whatever is the correct
 Forth cell size).  

 prslw()
 {
       unsigned cell buffer, addr; /* M002 */ /* M003 */
 #ifdef SLOWSTACK /* M004 */
       cell pop(); /* M003 */
 #endif /* SLOWSTACK -- M004 */
       cell flag; /* M003 */
       int i, temp, unwrittenflag = FALSE; /* M000 */
       long fpos, ftell();
       char buf[1024];         /* holds data for xfer */

       flag = pop();
       buffer = pop();
       addr = pop();
       fpos = (long) (buffer * 1024);

                                       /* extend if necessary */
       if (fpos >= bfilesize) {
           if (flag == 0) {            /* write */
               printf("Extending block file to %ld bytes\n", fpos+1024);
/* M00[K1 */
               /* the "2" below is the fseek magic number for "beyond end" */
               fseek(blockfile, (fpos+1024) - bfilesize, 2);
               bfilesize = ftell(blockfile);
           }
           else {                      /* reading unwritten data */
               unwrittenflag = TRUE;   /* will read all zeroes */
           }
       }
       else {
               /* note that "0" below is fseek magic number for "relative to
                  beginning-of-file" */
               fseek(blockfile, fpos, 0);      /* seek to destination */
       }

       if (flag) {             /* read */
           if (unwrittenflag) {        /* not written yet */
               for (i=0; i<1024; i++) mem[addr++] = 0; /* "read" nulls */
           }
           else {                      /* does exist */
               if ((temp = fread (buf, sizeof(char), 1024, blockfile)) 
                                                               != 1024)
{
                       fprintf (stderr,
                               "File read error %d reading buffer %d\n",
                                       temp, buffer);
                       errexit();
               }
               for (i=0; i<1024; i++) mem[addr++] = buf[i];
           }
       }
       else {  /* write */
               for (i=0; i<1024; i++) buf[i] = mem[addr++];
               if ((temp = fwrite (buf, sizeof(char), 1024, blockfile))
                                                                != 1024)
{
                           fprintf(stderr,
                               "File write error %d writing buffer %d\n",
                                       temp, buffer);
                           errexit();
               }
       }
 }


 -- 
 Marc de Groot (KG6KF)                   These ARE my employer's opinions!
 Noe Systems, San Francisco
 UUCP: uunet!hoptoad!noe!marc
 Internet: marc@kg6kf.AMPR.ORG
 ------------
