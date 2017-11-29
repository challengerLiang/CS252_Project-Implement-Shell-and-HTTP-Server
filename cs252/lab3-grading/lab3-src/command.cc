
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#define MAXARGLENGTH 1024 //add by myself

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include <fcntl.h>
#include <regex.h>
#include <pwd.h>
//#include <dirent.h>


#include "command.h"

extern char **environ;
extern char * lastCommand;
extern char * lastSimpleCommand;
char chrForPid[100];
char * tempArg2;

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

    //add code myself
    //implement variable expansion
    char * buffer = (char *)"^.*${[^}][^}]*}.*$";
    regex_t re;
    regmatch_t match;
    int expbuf = regcomp(&re, buffer, 0);
    if (expbuf != 0)
    {
        perror("compile");
        exit(1);
    }

    //expand the variable
    if (regexec(&re, argument, 1, &match, 0) == 0)
    {
       char * str = (char *)malloc(sizeof(char) * MAXARGLENGTH);
       int i = 0;
       int j = 0;
       //find the place of the '$''
       while(i < MAXARGLENGTH  && argument[i] != '\0')
       {
           if (argument[i] != '$')
           {
               str[j] = argument[i];
               i++;
               j++;
           }
           else
           {
                //argument[i] == '$'
                //find where '{' and '}' is
                char * begin = strchr((char *)(argument + i), '{');
                char * end = strchr((char *)(argument + i), '}');
                //printf("%s\n", begin );
                //printf("%s\n", end);
                //copy the enviornment variable
                char * variable = (char *)malloc(MAXARGLENGTH * sizeof(char));
                //escape '{' and '}'
                strncpy(variable, begin + 1, (end - 1) - begin);
                variable[end - 1 - begin] = '\0';
                char * contentOfVariable;
                if (strcmp(variable, "_") == 0)
                    contentOfVariable = lastCommand;
                else if (strcmp(variable, "?") == 0)
                    contentOfVariable = (char *)"0";
                else if (strcmp(variable, "$") == 0)
                {
                    int pid = getpid();
                    snprintf(chrForPid, 99, "%d", pid);
                    contentOfVariable = chrForPid;
                }
                else if (strcmp(variable, "SHELL") == 0)
                    contentOfVariable = tempArg2;
                else    
                    contentOfVariable = getenv(variable);

                if (contentOfVariable != NULL)
                    strcat(str, contentOfVariable);
                else
                {
                    contentOfVariable = (char *)"";
                    strcat(str, "");
                }
                 //plus the length of varaible and 3 to original arg
                 i = i + strlen(variable) + 3;
                 //plus the length of contentOfVariable to the expanedArgument
                 j = j + strlen(contentOfVariable);
                 free(variable);
           }
       }
       str[j] = '\0';
       argument = strdup(str);
       //printf("%s\n",argument);
    }

    //tilde expansion
    if (argument[0] == '~')
    {
        if (strlen(argument) == 1)
            argument = strdup(getenv("HOME"));
        else
            argument = strdup(getpwnam(argument + 1)->pw_dir);
    }
    //add code myself end

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

	_append = 0;
	_ambiguousOut = 0;
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

	_append = 0;
	_ambiguousOut = 0;
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
		printf("\n");      /*This line was added by myself*/
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

	//implement Builtin command
   if (strcmp(_simpleCommands[0]->_arguments[0],"exit") == 0)
       {
       	   //printf("Good bye!!\n");
           exit(1);
       } 

	// Print contents of Command data structure
	//print();    //Comment out by myself

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	// add my own code here
    //save in/out/error
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperror = dup(2);
    int fdin;
    if (_inFile) 
    {
    	//open the file just for reading
        fdin = open(_inFile, O_RDONLY);
        if (fdin < 0)
        {
        	perror("");
        	clear();
            // Print new prompt
	        prompt();
	        return;
        }
    }
    else
    {
        //use default input
        fdin = dup(tmpin);	
    }

	int ret;
	int fdout;
	for (int i = 0; i < _numOfSimpleCommands; i++)
	{
        //redirect input
        dup2(fdin, 0);
        close(fdin);
        //setup output
        if (i == _numOfSimpleCommands - 1)
        {
        	//last simple command
        	if (_outFile)
        	{
        		if (_ambiguousOut)
        		{
        			perror("Ambiguous output redirect");
        			clear();
                    // Print new prompt
	                prompt();
	                return;
        		}
        		if (_append)
        		{
        		    //_append = 1, append new content 
        		    fdout = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0664);
        	    }
        	    else
        	    {
        		    //create a file that allows read and write
        		    fdout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
        	    }
        	}
        	else
        	{
        		//use default output
        		fdout = dup(tmpout);
        	}
        }
        else
        {
        	//not last simple command. So, pipe need be created
        	int fdpipe[2];
        	pipe(fdpipe);
        	fdout = fdpipe[1];
        	fdin = fdpipe[0];
        }

        //redirected output
        dup2(fdout, 1);
        //when _errFle = 1, redirect the error to the same location as output
        if (_errFile)
        {
        	dup2(fdout, 2);
        }
        close(fdout);

        if (strcmp(_simpleCommands[i]->_arguments[0],"setenv") == 0)
        {
            if (_simpleCommands[i]->_numOfArguments >= 2)
            {
                setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);
            	//printf("%s\n",_simpleCommands[i]->_arguments[3]);
            }  	
        }
        else if (strcmp(_simpleCommands[i]->_arguments[0],"unsetenv") == 0)
        	  unsetenv(_simpleCommands[i]->_arguments[1]);
        else if (strcmp(_simpleCommands[i]->_arguments[0],"cd") == 0)
        {
        	int dir;
        	if (_simpleCommands[i]->_numOfArguments > 1)
        	    dir = chdir(_simpleCommands[i]->_arguments[1]);
        	else
        	    dir = chdir(getenv("HOME"));  
        	//check dir
            if (dir != 0)
            {
        	    fprintf(stderr, "No such file or directory\n");
        	    exit(0);
            }  	
        }
   
        //create child process
		ret = fork();
	    if (ret == 0)
		{
			//child
			//check built in
            if (strcmp(_simpleCommands[i]->_arguments[0],"setenv") == 0)
            {
            	if (_simpleCommands[i]->_numOfArguments >= 2)
            	{
            		setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);
            		//printf("%s\n",_simpleCommands[i]->_arguments[3]);
            	}
            	else
            	{
            	    execlp("printenv","printenv",NULL);	
            	}
                exit(0);    	
            }
            else if (strcmp(_simpleCommands[i]->_arguments[0],"unsetenv") == 0)
            {
            	unsetenv(_simpleCommands[i]->_arguments[1]);
                exit(0);
            }
            else if (strcmp(_simpleCommands[i]->_arguments[0],"cd") == 0)
            {
            	if (_simpleCommands[i]->_numOfArguments > 1)
            	    chdir(_simpleCommands[i]->_arguments[1]);
            	else
        	        chdir(getenv("HOME"));
        	    exit(0);
            }
            //check ends
            else
            {
			    execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
                //if the child process reaches here, it meant that execvp failed
                perror("execvp");
                _exit(1);
            }
		}
		else if (ret < 0)
		{
			perror("fork");
			return;
		}
		// parent shell continue
	}

	//restore in/out defaults
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	dup2(tmperror, 2);
    //close
    close(tmpin);
    close(tmpout);
    close(tmperror);

	if (!_background)
	{
		//wait for last process
		waitpid(ret, NULL, 0);
	}
    //add my own code end
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	//use isatty() to check
	if (isatty(1) == 1)
	{
        if (getenv("PROMPT") == NULL)
        {
	        printf("myshell>");
	        fflush(stdout);
        }
        else
        {
            printf("%s>", getenv("PROMPT"));
            fflush(stdout);
        }
    }
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void disp(int signal)
{
    printf("\n");
    Command::_currentCommand.prompt();
}

void killzombie(int signal)
{
    //wait3(0,0,NULL);
    while(waitpid(-1, NULL, WNOHANG) > 0); 
}

int main(int argc, char const *argv[])
{
    //get argv[0]
    tempArg2 = (char *)argv[0];
	//ctrl C to exit
	struct sigaction sa;
    sa.sa_handler = disp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sa, NULL))
    {
        perror("sigaction");
        exit(2);
    }

    //zombie elimination
    struct sigaction signalAction;
    signalAction.sa_handler = killzombie;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;

    int error = sigaction(SIGCHLD, &signalAction, NULL );
    if (error)
    {
         perror("sigaction");
         exit(-1);
    }
    
    Command::_currentCommand.prompt();
	yyparse();    
}

