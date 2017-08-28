
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATAMP GREATGREAT GREATGREATAMP LESS PIPE AMPERSAND

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <assert.h>
#include "command.h"
#include <pwd.h>

void envExpansion(char*);
void tildeExpansion(char * arg);
void tildeOrEnv(char * arg);

char **array = NULL;
int nEntries = 0;
int maxEntries = 20;

void wildExpansionNecessary(char * arg);
void wildExpansion(char * prefix, char *suffix);
int compare(const void *s1, const void *s2);

void yyerror(const char * s);
int yylex();

%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: simple_command
        ;

simple_command:	
	//command_and_args iomodifier NEWLINE {
	pipe_list iomodifier_list background_optional NEWLINE {		
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE {	
		Command::_currentCommand.clear();
		Command::_currentCommand.prompt();
	}
	| error NEWLINE { yyerrok; }
	;

pipe_list://added
    pipe_list PIPE command_and_args
    | command_and_args
    ;

command_and_args:
	command_word argument_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

argument_list:
	argument_list argument
	| /* can be empty */
	;

argument:
	WORD {

		if (!(strchr($1, '*') == NULL && strchr($1, '?') == NULL)) {
			 wildExpansionNecessary($1);
		}
		else 
			tildeOrEnv($1);
		}
	;

command_word:
	WORD {
           //printf("   Yacc: insert command \"%s\"\n", $1);
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
       iomodifier_list iomodifier
       | /* can be empty */
       ;


iomodifier:
	GREAT WORD 
	{
	  	//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._outdirect++;
	}
	|
	GREATGREAT WORD 
	{
	 	//printf("   Yacc: append output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
		Command::_currentCommand._outdirect++;
	}
	|
	GREATAMP WORD 
	{
	  	//printf("   Yacc: insert error \"%s\"\n", $2);
		//Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._outdirect++;
	}
	|
	GREATGREATAMP WORD 
	{
	  	//printf("   Yacc: append error \"%s\"\n", $2);
		//Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._append = 1;
		Command::_currentCommand._outdirect++;
	}
	|
	LESS WORD 
	{
	  	//printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inFile = $2;
	}
	;

background_optional:
	AMPERSAND 
	{
	  	//printf("   Yacc: run in background YES\n");
		Command::_currentCommand._background = 1;
	}
	| 
	;

%%
#include <regex.h>
#include <dirent.h>


void tildeOrEnv(char * arg) {
	if(*arg == '~') { tildeExpansion(arg); }
	if(*arg == '$') { envExpansion(arg); }
	Command::_currentSimpleCommand->insertArgument(arg);
}

void tildeExpansion(char * arg){
	if ((strcmp(arg, "~") == 0) || (strcmp(arg, "~/") == 0)) {	
		strcpy(arg, getpwnam(getenv("USER"))->pw_dir);
	} else {
		char newarg[strlen(arg) + 10];
		strcpy(newarg, "/homes/");
		char *shiftarg = arg;
		shiftarg++;
		strcat(newarg,shiftarg);
		strcpy(arg,newarg);
	} 
} 

void envExpansion(char* arg) {
	char * tomatch = "\\${.*}";
	regex_t re;
	regmatch_t match;
	int regexbuff = regcomp(&re, tomatch, 0);

	if(!regexec(&re, arg, 1, &match, 0)) {
		char expandarg[1024];
		memset(expandarg, 0, 1024);
		char * temp = expandarg;
		for (int i = 0; arg[i]!='\0' && i < 1024; ) {
			if (arg[i] == '$') {
				char *beg = strchr((char *)(arg + i), '{');
				char *end = strchr((char *)(arg + i), '}');
				beg[end - beg] = '\0';
				beg++;
				char *out = strdup(beg);
				char * final = getenv(out);
				strcat (expandarg, final);
				temp = temp + strlen(final);
				i = i + strlen(out) + 3;
			}
			else {
				*temp = arg[i];
				temp++;
				i++;
			}
		}
		strcpy(arg,expandarg);
	}
}



void wildExpansionNecessary(char * arg) {

	char *temparg = strdup(arg);
	if (strchr(temparg, '*') == NULL && strchr(temparg, '?') == NULL && strchr(temparg, '~') == NULL){
		Command::_currentSimpleCommand->insertArgument(temparg);
		return;
	}

	wildExpansion("", temparg);

	if(nEntries > 1) { qsort(array, nEntries, sizeof(char*), compare); }

	for (int i = 0; i < nEntries; i++) { 
		Command::_currentSimpleCommand->insertArgument(array[i]);
	}

	if(array != NULL) { free(array); }
	array = NULL;
	nEntries = 0;
	return;

}

void wildExpansion(char * prefix, char *suffix) {
	if (suffix[0] == 0) { return; }

	bool flag = false;
	if(suffix[0] == '/') { flag = true; }

	char * s = strchr(suffix, '/');
	char component[1024];
 	for(int i = 0; i < 1024; i++) component[i] = 0; 
	if (s == NULL){
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
	}
	else { 
		if(prefix[0] == 0 && flag) {
			suffix = s+1;
			s = strchr(suffix, '/');

			if(s != NULL) {
				strncpy(component,suffix, s-suffix);
				suffix = s + 1;
				prefix = "/";

			}
			else {

				strcpy(component, suffix);
				suffix = suffix + strlen(suffix);
				prefix = "/";
			}
		}
		else {
			strncpy(component,suffix, s-suffix);
			suffix = s + 1;
		}
	}

	char tildePrefix[1024];
	for(int i = 0; i < 1024; i++) tildePrefix[i] = 0; 
	if(component[0] == '~') {
		struct passwd *pwd;
		if(strcmp(component, "~") == 0){ pwd = getpwnam(getenv("USER")); }
		else { pwd = getpwnam(component + 1); }

		if(pwd) { 
		  if(suffix[0] == 0 && prefix[0] == 0) { sprintf(tildePrefix,"%s",pwd->pw_dir); }
		  else if(suffix[0] == 0) 			   { sprintf(tildePrefix,"%s/%s",pwd->pw_dir, component); }
		  else if(prefix[0] == 0) 			   { sprintf(tildePrefix,"%s/%s",pwd->pw_dir, suffix); }
		  wildExpansionNecessary(tildePrefix);
		  return;

		}
		else if (!pwd) {
			printf("Could not access user %s.\n", component + 1); 
		}
	}

	char newPrefix[1024];
	for(int i = 0; i < 1024; i++) newPrefix[i] = 0; 

	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {

		if(prefix[0] == 0 && !flag) 		{ sprintf(newPrefix,"%s",component); }
		else if(strcmp(prefix, "/") == 0)   { sprintf(newPrefix,"/%s",component); }
		else 								{ sprintf(newPrefix,"%s/%s",prefix,component); }    

		wildExpansion(newPrefix, suffix);
		return;
	}

	char * reg = (char*)malloc(2*strlen(component)+10); 
	char * a = component;
	char * r = reg;
	*r = '^'; r++;

	for (; *a; a++) { 
		if (*a == '*') 		{ *r='.'; r++; *r='*'; r++; }
		else if (*a == '?') { *r='.'; r++;}
		else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
		else 				{ *r=*a; r++;}
	}
	*r = '$'; r++; *r = 0;

	regex_t re;
	regmatch_t match;	
	int expbuf = regcomp( &re, reg, REG_EXTENDED|REG_NOSUB);
	free(reg);
	if (expbuf != 0) {
		perror("compile");
		return;
	}
  
	char * d;
	if (prefix[0] == 0){ d = "."; }
	else { d = prefix; }

	DIR * dir = opendir(d);
	if (!dir) { return; }

	struct dirent * ent;
	if(!array) array = (char**) malloc(maxEntries*sizeof(char*));

	while ( (ent = readdir(dir))!= NULL) {

		if (regexec(&re, ent->d_name, 1, &match, 0 ) == 0) {

			if(array != NULL && nEntries > 1) qsort(array, nEntries, sizeof(char*), compare);
	
	    	if (nEntries == maxEntries) {
				maxEntries *=2; 
				array = (char**) realloc(array, maxEntries*sizeof(char*));
				assert(array!=NULL);
			}

			if (ent->d_name[0] != '.') {
				if(prefix[0] == 0) { sprintf(newPrefix,"%s",ent->d_name); }
				else if(prefix[0] == '/' && prefix[1] == 0) { sprintf(newPrefix,"/%s",ent->d_name); }
				else { sprintf(newPrefix,"%s/%s",prefix,ent->d_name); }
			
				wildExpansion(newPrefix,suffix);
				if(s == NULL) {
					array[nEntries]= strdup(newPrefix);
					nEntries++;
				}
			}
			else if (ent->d_name[0] == '.') {
				if (component[0] == '.') {
					if(prefix[0] == 0) { sprintf(newPrefix,"%s",ent->d_name); }
					else if(prefix[0] == '/' && prefix[1] == 0) { sprintf(newPrefix,"/%s",ent->d_name); }
					else { sprintf(newPrefix,"%s/%s",prefix,ent->d_name); }
		
					wildExpansion(newPrefix,suffix);
					if(!s) {
						array[nEntries]= strdup(newPrefix);
						nEntries++;
					}
				}
			}	
		}
	}
	closedir(dir);
	return; 
}

int compare(const void *s1, const void *s2) {
	return strcmp(*(char *const*)s1, *(char *const*)s2);
}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
