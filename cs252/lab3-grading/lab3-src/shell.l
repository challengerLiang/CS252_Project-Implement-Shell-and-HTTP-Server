
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

#include <regex.h>

#include <sys/wait.h>

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

////////////  Start added code ///////////
extern "C" char * read_line();

char * read_line();
int mygetc(FILE * f) {
static char *p;
char ch;
if (!isatty(0)) {
  // stdin is not a tty. Call real getc
  return getc(f);
}
// stdin is a tty. Call our read_line.
if (p==NULL || *p == 0) {
  char * s = read_line();
  p = s;
}
ch = *p;
p++;
return ch;
}
#undef getc
#define getc(f) mygetc(f)
/////////// End added code ///////////

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



">>"  {
		  return GREATGREAT;
	}

"|"   {
		  return PIPE;
	}

"&"   {
	      return AMPERSAND;
	}	

"<" 	{
		return LESS;
	}

">&"   {
	   return GREAT_AMPERSAND;

    }

 ">>&"  {
 	    return GREATGREAT_AMPERSAND;
    }  

[^ \t\n]*(\\[^\s])[^ \t\n]*  {
	/*escape character*/
	/*printf("success catch escape character\n");*/
    char * temp = (char *)malloc(sizeof(char *) * (strlen(yytext) + 10));
    /* found escape character */
    int found = 0;
    int j = 0;
    for (int i = 0; i < strlen(yytext); i++)
    {
        if (found == 0 && yytext[i] == '\\')
            found = 1;
        else if (found == 1)
        {
        	found = 0;
        	temp[j] = yytext[i];
        	j++;
        }
        else
        {
        	temp[j] = yytext[i];
        	j++;
        }
    }
    temp[j] = '\0';
    yytext = strdup(temp);
    free(temp);

    yylval.string_val =strdup(yytext);
	return WORD;    
}

\"[^\n\"]*\" {
    /* code for escaping quote */
    yytext = (char *)(yytext + 1);
    yytext[strlen(yytext) - 1] = '\0';
    yylval.string_val =strdup(yytext);
    return WORD;
}


[^ ^\\&<>|\t\n][^ ^\\&<>|\t\n]*  {    /*this line was changed by myself*/
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

\`[^\n\`]*\`    {
    /* subshell */
    yylval.string_val = strdup(yytext);

    return SUBSHELL;
}




