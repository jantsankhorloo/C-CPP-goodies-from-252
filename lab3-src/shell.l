
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <unistd.h>
#include "y.tab.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>


static  void yyunput (int c,char *buf_ptr  );
void escape_chars();
void subshell();
char * read_line();

char * shelldir;

/*int mygetc(FILE * f) 
{
	static char *p;
	char ch;

	if (!isatty(0)) 
	{
		// stdin is not a tty. Call real getc
		return getc(f);
	}

	// stdin is a tty. Call our read_line.

	if (p==NULL || *p == 0) 
	{
		char * s = read_line();
		p = s;
	}

	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)
*/

void myunputc(int c) {
    unput(c);
}



%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

">&" 	{
		return GREATAMP;
	}

">>" 	{
		return GREATGREAT;
	}

">>&" 	{
		return GREATGREATAMP;
	}

"<" 	{
		return LESS;
	}

"|" 	{
		return PIPE;
	}

"&" 	{
		return AMPERSAND;
	}

[^ \t\n]*(\\[^\s])[^ \t\n]*	{
		escape_chars();
		yylval.string_val = strdup(yytext);
		return WORD;
	}

\"[^\n\"]*\"	{
		char* noquote = yytext;
		noquote++;
		yytext = noquote;
		yytext[strlen(yytext)-1] = '\0';
		yylval.string_val = strdup(yytext);	
		return WORD;
	}

[^ \t\n<>&\|]*(\\.[^ \t\n<>&\|]*)*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}
%%

void escape_chars() {
	char* yytext2 = (char*)malloc(strlen(yytext)+1);
	int j = 0, flag = 0; //escaped?
	
	for(int i = 0; i < strlen(yytext); i++) {
		if(yytext[i] == '\\' && flag == 0) { flag = 1; }
		else if(flag == 1) {
			yytext2[j] = yytext[i];
			j++;
			flag = 0;
		}
		else {
			yytext2[j] = yytext[i];
			j++;
		}
	}
	yytext2[j] = '\0';
	strcpy(yytext, yytext2);
	free(yytext2);
}
