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

%token	<string_val> WORD SUBSHELL

%token 	NOTOKEN GREAT NEWLINE GREATGREAT PIPE AMPERSAND LESS GREAT_AMPERSAND GREATGREAT_AMPERSAND   /*have modefied by myself*/

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#define MAXFILENAME 1024

#include <stdio.h>
#include <string.h>
#include "command.h"

#include <regex.h>
#include <dirent.h>
#include <stdlib.h>
#include <assert.h> 
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

void yyerror(const char * s);
int yylex();

//declare variable
//indicator of expand wildcard or not
int expanded = 0;
//initially allocate 10
int maxEntry = 10;
int argNum = 0;
char ** argArray = (char **)malloc(sizeof(char *) * maxEntry);

int count = 0;
int specialCase = 0;

int specialWildcardFlag = 1;

extern void myunputc(int c);

//implment expandWiledcards
void expandWildcard(char * prefix, char * suffix)
{
   //special for ${?}
   if (count == 0 && (strstr(suffix, "${?}") != NULL))
       specialWildcardFlag = 0;
    //special for ${?} end
   
    count++;
    if (suffix[0] == '/' && count == 1)
    {
        char temp[MAXFILENAME];   
        char * str = strchr((char*)(suffix + 1), '/');
        if (str != NULL)
            strncpy(temp, suffix, str - suffix);
        else
            strcpy(temp, suffix);
        if (strchr(temp, '*') != NULL)
        {
            //special condition like /u*/* was found
            //deal with the sepcial case
             specialCase = 1;
            //expandWildcard(NULL, strdup(str + 1));
            expandWildcard(strdup((char *)""), strdup(suffix + 1));
            return;
        }     
    }

    if (suffix[0] == 0)
    {
        //suffix is empty. Put prefix in argArray.
        //expand argArray if needed
        if (argNum == maxEntry)
        {
            //realloc
            maxEntry = 2 * maxEntry;
            argArray = (char**)realloc(argArray, maxEntry*sizeof(char*));
            assert(argArray != NULL);
        }
        //Command::_currentSimpleCommand->insertArgument(strdup(prefix));
        argArray[argNum] = strdup(prefix);  
        argNum++;
        return; 
    }

   /* if (strchr(suffix, '*') || strchr(suffix, '?'))
        expanded = 1;*/
    // Obtain the next component in the suffix
    // Also advance suffix.
    //initialize s
    char * s = NULL;
    if (suffix[0] == '/')
        s = strchr((char*)(suffix + 1), '/');
    else
        s = strchr(suffix, '/');
    char component[MAXFILENAME];
    if (s != NULL)
    { //Copy up to the first “/”
        strncpy(component,suffix, s - suffix);
        suffix = s + 1;
    }
    else 
    { 
        // Last part of path. Copy whole thing.
        strcpy(component, suffix);
        suffix = suffix + strlen(suffix);
    }

    // Now we need to expand the component
    char newPrefix[MAXFILENAME];
    if ((strchr(component, '*') == NULL && strchr(component, '?') == NULL)
        || specialWildcardFlag == 0)
    {
        //component does not have wildcards
        //check whether the prefix is empty
        //printf("prefix = %s\n",prefix);
        if (prefix == NULL)
            sprintf(newPrefix, "%s", component);
        else
            sprintf(newPrefix, "%s/%s", prefix, component);
        expandWildcard(newPrefix, suffix);
        return;
    }
    // Component has wildcards
    // Convert component to regular expression
    char * reg = (char*)malloc(2*strlen(component) + 10);
    char * a = component;
    char * r = reg;
    *r = '^';
    r++;
    while (*a != '\0')
    {
        if (*a == '*')
        {
            *r = '.';
            r++;
            *r = '*';
            r++;
        }
        else if (*a == '?')
        {
            *r = '.';
            r++;
        }
        else if (*a == '.')
        {
            *r ='\\';
            r++;
            *r = '.';
            r++;
        }
        else
        {
            *r = *a;
            r++;
        }
        a++;
    }
    *r = '$';
    r++;
    *r = '\0';
    //compile regular expression
    regex_t re;
    int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
    if (expbuf != 0)
    {
        perror("compile");
        return;
    }

    char * dirName;
    //if prefix is empty, then list directory
    if (prefix == NULL)
        dirName = (char *)".";
    else
        dirName = prefix;

    if (specialCase == 1)
    {
        dirName = ((char *)"/");
        //special case has effected
        specialCase = 2;
    }

    DIR * dir = opendir(dirName);
    if (dir == NULL)
        return;
    struct dirent *entry;
    regmatch_t match;
    while ((entry = readdir(dir)) != NULL)
    {
        //check if name matches
        //this line may need to be modefied
        if (regexec(&re, entry->d_name, 1, &match, 0) == 0)
        {
            //Entry matches. Add entry that matches to the prefix
            //and call expandWildcard recursively

            //Command::_currentSimpleCommand->insertArgument(strdup(entry->d_name));


            if (entry->d_name[0] == '.')
            {
                //printf("%s\n", entry->d_name);
                if (component[0] == '.')
                {
                    if (prefix == NULL)
                        sprintf(newPrefix, "%s", entry->d_name);
                    else
                        sprintf(newPrefix, "%s/%s", prefix, entry->d_name);
                    expandWildcard(newPrefix, suffix);
                }                              
            }
            else
            {
                if (prefix == NULL)
                    sprintf(newPrefix, "%s", entry->d_name);
                else
                    sprintf(newPrefix, "%s/%s", prefix, entry->d_name);
                expandWildcard(newPrefix, suffix);
                /*argArray[argNum] = strdup(entry->d_name);
                argNum++;*/
            }
            //printf("%s\n",entry->d_name);
            
        } 
    }
    closedir(dir);
}

void resetArray()
{
    //reset array
    free(argArray);
    argNum = 0;
    maxEntry = 10;
    argArray = (char **)malloc(sizeof(char *) * maxEntry);
    //expanded = 0;

}

int compare (const void *a, const void *b)
{
    return strcmp(*(const char**)a, *(const char**)b);
}


void sortArgs()
{
    qsort(argArray, argNum, sizeof(const char *), compare);    
}
//implement end

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

	       //Command::_currentSimpleCommand->insertArgument( $1 );
          
           count = 0;
           specialCase = 0;
	       expandWildcard(NULL, $1);
           if (specialWildcardFlag == 0)
               specialWildcardFlag = 1;
           count = 0;
           specialCase = 0;
           
           //sort the array if expanded = 1
           /* if (expanded == 1)
           {*/
               sortArgs();
               for (int i =0; i < argNum; i++)
               {
                   Command::_currentSimpleCommand->insertArgument(argArray[i]); 
               }   
            resetArray(); 
           //}
	}
    |   SUBSHELL  {
        //deal with subshell
        //define two pipes
        int pipeParent[2];
        int pipeChild[2];
        #define readByChild pipeChild[0]
        //parent writes into child preocess to let the child read
        //write by parent where child will read from
        #define writeByParent pipeChild[1]
        #define readByParent pipeParent[0]
        //child writes into parent process to let the child read
        //write by child where parent will read from
        #define writeByChild pipeParent[1]
        //get correct args
        char * arg = $1;
        arg = (char *)(arg + 1);
        arg[strlen(arg) - 1] = '\0';
        char * argument = (char *)malloc(sizeof(char *) * (strlen(arg)));
        //char * arg2 = argument;
        //strcpy(argument, arg);
        //strcat(argument, "\nexit\n");

        //store stdin, stdout
        int tempIn = dup(0);
        int tempOut = dup(1);
        //initalize maximum length
        int maxLength = 20;
        pipe(pipeParent);
        pipe(pipeChild);
        char * str = (char *)malloc(sizeof(char) * 20);
        //char * str2 = str;
        char chr = '\0';
        int count = 0;

        //write exit information to child process
        write(writeByParent, arg, strlen(arg));
        write(writeByParent, "\nexit\n", 6);
        close(writeByParent);
        //redirect stdout and stdin
        dup2(readByChild, 0);
        dup2(writeByChild, 1);
        close(readByChild);
        close(writeByChild);

        //fork
        int ret = fork();
        if (ret < 0)  
        {
            perror("fork");
            exit(1);
        }
        else if (ret == 0)
        {
            //child process

            char *tempArg[2];
            tempArg[0] = (char *)"/proc/self/exe";
            tempArg[1] = NULL;
            execvp(tempArg[0], tempArg);
            perror("subshell");
            _exit(0);
        }


        //restore stdout and stdin
        dup2(tempOut, 1);
        dup2(tempIn, 0);
        close(tempOut);
        close(tempIn);
        if (read(readByParent, &chr, 1) < 0)
            perror("Error in read");

        str[0] = chr;
        count++;
        while(read(readByParent, &chr, 1) != 0)
        {
            if (chr != '\n')
                str[count] = chr;
            else
                str[count] = ' ';
            if(count == maxLength)
            {
                //expand maxLength
                maxLength = 2 * maxLength;
                str = (char*)realloc(str, maxLength * sizeof(char*));
            }
        count++;
        }

        //call waitpid here
        waitpid(ret, NULL, 0);
        
        //inverse the order
        int i = 0;
        while (i < count)
        {
            //use myunputc function defined at shell.l to inverse the order
            myunputc(str[count - 1 - i]);
            i++;
        }
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
