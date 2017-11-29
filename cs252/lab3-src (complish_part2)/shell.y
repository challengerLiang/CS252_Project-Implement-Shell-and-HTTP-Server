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
 %error-verbose

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATGREAT PIPE AMPERSAND LESS GREAT_AMPERSAND GREATGREAT_AMPERSAND   /*have modefied by myself*/

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <string.h>
#include "command.h"
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
	pipe_opt iomodifier_list background_opt NEWLINE {
		/* printf("   Yacc: Execute command\n"); */
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

pipe_opt:
    pipe_opt PIPE command_and_args {
        /* printf("Recognize pipe command\n");  */
    } 
    |  command_and_args
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
              /* printf("   Yacc: insert argument \"%s\"\n", $1); */

	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

command_word:
	WORD {
              /* printf("   Yacc: insert command \"%s\"\n", $1); */
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
     iomodifier_opt | iomodifier_list iomodifier_opt | /* can be empty */

iomodifier_opt:
	GREAT WORD {
		/* printf("   Yacc: insert output \"%s\"\n", $2); */
		if (Command::_currentCommand._outFile != 0)
            Command::_currentCommand._ambiguousOut = 1;
		Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		/* printf("   Yacc: insert input \"%s\"\n", $2); */
		Command::_currentCommand._inFile = $2;
	}
	| GREAT_AMPERSAND WORD{
	    /* printf("   Yacc: insert output and error message \"%s\"\n", $2); */
	    if (Command::_currentCommand._outFile != 0)
            Command::_currentCommand._ambiguousOut = 1;
	    Command::_currentCommand._outFile = strdup($2);
	    Command::_currentCommand._errFile = $2;
	}
	| GREATGREAT WORD {
		/* printf("   Yacc: insert output \"%s\"\n", $2); */
		if (Command::_currentCommand._outFile != 0)
            Command::_currentCommand._ambiguousOut = 1;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
		/* printf("   append =  \"%d\"\n", Command::_currentCommand._append); */
    }
    | GREATGREAT_AMPERSAND WORD {
        /* printf("   Yacc: insert output and error message \"%s\"\n", $2); */
        if (Command::_currentCommand._outFile != 0)
            Command::_currentCommand._ambiguousOut = 1;
	    Command::_currentCommand._outFile = strdup($2);
	    Command::_currentCommand._errFile = $2;
		Command::_currentCommand._append = 1;
		/*  printf("   append =  \"%d\"\n", Command::_currentCommand._append);  */
    }
	;

background_opt:
    AMPERSAND{
        /* printf("Yacc: execute in background"); */
        Command::_currentCommand._background = 1;
    } 
    |
    ;



	

%%

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
