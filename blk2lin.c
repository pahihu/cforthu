/* usage: block2line < blockfile > linefile
 * takes a block file from stdin and makes a cr-delimited file to stdout
 * with 64 characters per line, 16 lines per screen
 */

#include <stdio.h>
#include <stdlib.h>

int main()
{
	int i, j, screen;
	char buf[64];	/* max line size */

        screen = 0;
	while(1) {
	    printf("------------------ SCREEN %d ------------------\n",
			screen++);
	    for (i=0; i<16; i++) {
		if (fread(buf,sizeof(char),64,stdin) < 64) exit(0);
		j = 63;
		while (buf[j] == ' ' && j >= 0) j--;
		if (j >= 0) fwrite(buf,sizeof(char),j+1,stdout);
		putchar('\n');
	    }
	}
        return 0;
}
