
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_outdirect = 0;
	_append = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_outdirect = 0;
	_append = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	// Print contents of Command data structure
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	if (_outdirect > 1) {
		printf("Ambiguous output redirect");
	}
	if(_numOfSimpleCommands == 1 && strcmp( _simpleCommands[0]->_arguments[0], "exit" )== 0) {
	    printf("\n Good bye!!\n\n");
	    exit(0);
	}
	if(strcmp( _simpleCommands[0]->_arguments[0], "cd" ) == 0) {
	    int tmp;
	    if(_simpleCommands[0]->_numOfArguments == 1) {
	      tmp = chdir(getenv("HOME"));
	    
		} else {
	      tmp = chdir(_simpleCommands[0]->_arguments[1]);
		}
	    if(tmp < 0){ perror("cd"); }
	    clear();
	    prompt();
	    return;
	}

	int tempin = dup(0);//creating new file descriptors, points to the same file object the arg is pointing to 
	int tempout = dup(1);
	int temperr = dup(2);
	
	int fdin, fderr;
	
	if(_inFile) {
	    fdin = open(_inFile, O_RDONLY);//read only -rw-r--r--
	}
	else {
	    // use default input
	    fdin = dup(tempin);
	}
	if(_errFile){
		if (_append) {
			fderr = open(_errFile, O_CREAT|O_WRONLY|O_APPEND, 0664);
		}
		else {
			fderr = open(_errFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);		
		}
	}
	else {
		// use default error
		fderr = dup(temperr);
	}

	int ret, fdout;
	for (int i = 0; i < _numOfSimpleCommands; i++) {
		dup2(fdin, 0);//0 refers to the same open file object fdin refers to 
		dup2(fderr, 2);
		close(fdin);
		close(fderr);
		if (i == _numOfSimpleCommands - 1) {
			if (_outFile) {
				if (_append == 0) {
					fdout = open(_outFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
				}
				else {
					fdout = open(_outFile, O_CREAT|O_WRONLY|O_APPEND, 0664);		
				}
			}
			else {
				fdout = dup(tempout);
			}
		}
		else {
			int fdpipe[2];
			pipe(fdpipe);//after calling pipe, fdpipe will contain 2 file descriptors that point to 2 open file objects that are interconnected. what is written into fdpipe[1] can be read from fdpipe[0]
			fdout = fdpipe[1];
			fdin = fdpipe[0];		
		}
		dup2(fdout, 1);
		close(fdout);
		//PART 3
		if (strcmp(_simpleCommands[0]->_arguments[0], "setenv") == 0) {
			if(getenv(_simpleCommands[i]->_arguments[1])) {
				setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);
			}
			else {
				setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 0);
			}
		}
		else if(strcmp(_simpleCommands[i]->_arguments[0], "unsetenv") == 0) {
			unsetenv(_simpleCommands[i]->_arguments[1]);
		}
		else if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
			if(_simpleCommands[0]->_arguments[1]) {
				chdir(_simpleCommands[0]->_arguments[1]);
			}
			else {
				char *dir = getenv("HOME");
				chdir(dir);
			}
			clear();
			prompt();
			return;
		} 
		else {
			ret = fork();
			if (ret == 0) {
				if(strcmp( _simpleCommands[i]->_arguments[0], "printenv" ) == 0) {
					char **p = environ;
					for (;*p != NULL;p++) {
					  printf("%s\n",*p);
					}
					exit(0);
				}
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("execvp");
				_exit(1);
			}
			else if (ret < 0) {
				perror("fork");
				return;
			}
		}
	}
	
	dup2(tempin, 0);
	dup2(tempout, 1);
	dup2(temperr, 2);
	close(tempin);
	close(tempout);
	close(temperr);
	
	if (!_background) {
		waitpid(ret, NULL, 0);
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
  if (isatty(0)) {
	printf("myshell > ");
	fflush(stdout);
  }
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);


void ctrl_c(int sig_int=0) {
	fprintf(stdout,"\n");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}

void zombie(int signal)
{
  int status;
  pid_t	pid;
  
  while (1) {
    pid = wait3(&status, WNOHANG, (struct rusage *)NULL);
    if (pid == 0 || pid == -1)
      return;
  }
}
main()
{
	signal(SIGINT, ctrl_c);
	signal(SIGCHLD, zombie);
	Command::_currentCommand.prompt();
	yyparse();
}

