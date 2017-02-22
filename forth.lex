%{
/* LEX input for FORTH input file scanner */
/* 
	Specifications are as follows:
	This file must be run through "sed" to change 
		yylex () {
	to
		TOKEN *yylex () {
	where the sed script is
		sed "s/yylex () {/TOKEN *yylex () {/" lex.yy.c

	Note that spaces have been included above so these lines won't be
	mangled by sed; in actuality, the two blanks surrounding () are
	removed.

	The function "yylex()" always returns a pointer to a structure:

	    struct tokenrec {
		int type;
		char *text;
	    }
	    #define TOKEN struct tokenrec

	where the type is a hint as to the word's type:
		DECIMAL for decimal literal		d+
		OCTAL for octal literal		0d*
		HEX for hex literal		0xd+ or 0Xd+
		C_BS for a literal Backspace	'\b'
		C_FF for a literal Form Feed	'\f'
		C_NL for a literal Newline	'\n'
		C_CR for a literal Carriage Return '\r'
		C_TAB for a literal Tab '\t'
		C_BSLASH for a literal backslash '\\'
		C_IT for an other character literal 'x' where x is possibly '
		STRING_LIT for a string literal (possibly containing \")
		COMMENT for a left-parenthesis (possibly beginning a comment)
		PRIM for "PRIM"
		CONST for "CONST"
		VAR for "VAR"
		USER for "USER"
		LABEL for "LABEL"
		COLON for ":"
		SEMICOLON for ";"
		SEMISTAR for ";*" (used to make words IMMEDIATE)
		NUL for the token {NUL}, which gets compiled as a null byte;
			this special interpretation takes place in the COLON
			code.
		LIT for the word "LIT" (treated like OTHER, except that
			no warning is generated when a literal follows this)
		OTHER for an other word not recognized above

	Note that this is just a hint: the meaning of any string of characters
	depends on the context.

*/
%}

decimal	[0-9]
hex	[0-9A-Fa-f]
octal	[0-7]
white	[ \t\n\r\f]

%{
#include "forth.lex.h"
TOKEN token;

#define YY_INPUT(buf,result,max_size) \
        { \
        int c = getchar(); \
        result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
        }
%}

%%
{white}*	/* whitespace -- keep looping */ ;

-?[1-9]{decimal}*/{white}	{ token.type = DECIMAL; token.text = yytext;
					return &token; }
-?0{octal}*/{white}		{ token.type = OCTAL; token.text = yytext;
					return &token; }
-?0[xX]{hex}+/{white}		{ token.type = HEX; token.text = yytext;
					return &token; }

\'\\b\'/{white}	{ token.type = C_BS; token.text = yytext; return &token; }
\'\\f\'/{white}	{ token.type = C_FF; token.text = yytext; return &token; }
\'\\n\'/{white}	{ token.type = C_NL; token.text = yytext; return &token; }
\'\\r\'/{white}	{ token.type = C_CR; token.text = yytext; return &token; }
\'\\t\'/{white}	{ token.type = C_TAB; token.text = yytext; return &token; }
\'\\\\\'/{white} { token.type = C_BSLASH; token.text = yytext; return &token; }
\'.\'/{white}	{ token.type = C_LIT; token.text = yytext; return &token; }

\"(\\\"|[^"])*\"/{white} { token.type = STRING_LIT; token.text = yytext; 
				return &token; }

"("/{white}		{ token.type = COMMENT; token.text = yytext;
				return &token; }

"PRIM"/{white}		{ token.type = PRIM; token.text = yytext;
				return &token; }

"CONST"/{white}		{ token.type = CONST; token.text = yytext;
				return &token; }

"VAR"/{white}		{ token.type = VAR; token.text = yytext;
				return &token; }

"USER"/{white}		{ token.type = USER; token.text = yytext;
				return &token; }

"LABEL"/{white}		{ token.type = LABEL; token.text = yytext;
				return &token; }

":"/{white}		{ token.type = COLON; token.text = yytext;
				return &token; }

";"/{white}		{ token.type = SEMICOLON; token.text = yytext;
				return &token; }

";*"/{white}		{ token.type = SEMISTAR; token.text = yytext;
				return &token; }

"{NUL}"/{white}		{ token.type = NUL; token.text = yytext;
				return &token; }

"LIT"/{white}		{ token.type = LIT; token.text = yytext;
				return &token; }

[^ \n\t\r\f]+/{white}	{ token.type = OTHER; token.text = yytext;
				return &token; }
%%
