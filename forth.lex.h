/* this is my best effort at a reconstruction of this file - it was not
**  included with the distribution, and I cannot reach the author via
**   electronic mail!
** John Nelson  (decvax!genrad!john)  [moderator, mod.sources]
*/

struct tokenrec {
    int type;
    char *text;
};

#define TOKEN struct tokenrec

TOKEN *yylex();

#define DECIMAL		1
#define OCTAL		2
#define HEX		3
#define C_BS		4
#define C_FF		5
#define C_NL		6
#define C_CR		7
#define C_TAB		8
#define C_BSLASH	9
#define C_LIT		10
#define STRING_LIT	11
#define COMMENT		12
#define PRIM		13
#define CONST		14
#define VAR		15
#define USER		16
#define LABEL		17
#define COLON		18
#define SEMICOLON	19
#define SEMISTAR	20
#define NUL		21
#define LIT		22
#define OTHER		23
