/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;

int sizeOfHistory = 20;
char ** history = NULL;
char * lastCommand = "";
//this is for {?}
char * lastSimpleCommand = "";
/*char * history [] = {
  "ls -al | grep x", 
  "ps -e",
  "cat read-line-example.c",
  "vi hello.c",
  "make",
  "ls -al | grep xxx | grep yyy"
};*/
int history_length = 0;

//int upArrow = 0;
//int downArrow = 0;
int arrowNum = 0;

int position;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?                Print usage\n"
    " up arrow              See last command in the history\n"
    " down arrow            See next command in the history\n"
    " left arrow            Move cursor to the left (Allow insertion at the position)\n"
    " right arrow           Move cursor to the right(Allow insertion at the position)\n"
    " Delete key(ctrl-D)    Removes the character at the cursor\n"
    " Backspace key(ctrl-H) Removes the character at the position before the cursor.\n" 
    " Home key(ctrl-A)      The cursor move to the beginning of the line\n"
    " End key(ctrl-E)       The cursor moves to the end of the line\n";
    


  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  if (history == NULL)
      history = (char**) malloc(sizeof(char*) * sizeOfHistory);

  //initalize line_length and position
  line_length = 0;
  position = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 
   
      if (position == line_length)
      {
          // add char to buffer.
          line_buffer[line_length]=ch;
          line_length++;
      }
      else
      {
          //position != line_length
          //insert the character in the middle
          line_length++;
          int num = line_length;

          while (num > position)
          {
              line_buffer[num] = line_buffer[num - 1];
              num--;
          }
          line_buffer[position] = ch;
          
          //let users to see that they have actually insert some characters when typing
          //write the inserted character and the character behind to the screen
          num = position + 1;
          while (num < line_length)
          {
              ch = line_buffer[num];
              write(1, &ch, 1);
              num++;
          }
          //move the cursor back to the place right place
          //set character to backspace
          ch = 8;

          num = position + 1;
          while (num < line_length)
          {
              write(1, &ch, 1);
              num++;
          }
      }
      position = position + 1;
    }
    else if (ch==10) {
      // <Enter> was typed. Return line

      // Print newline
      write(1,&ch,1);

      //insert the line to history table
      //expand history table if needed
      if (history_length == sizeOfHistory)
      {
          sizeOfHistory = 2 * sizeOfHistory;
          history = (char**)realloc(history, sizeof(char*) * sizeOfHistory); 
      }
      line_buffer[line_length] = '\0';
      if (line_buffer[0] != '\0')
      {
          history[history_length] = strdup(line_buffer);
          if (history_length - 1 >= 0)
              lastCommand = strdup(history[history_length - 1]);

          int i;
          for (i = history_length - 1; i>=0; i--)
              if (strchr(history[i], '&') == NULL)
              {
                //'&' not found
                lastSimpleCommand = strdup(history[i]);
                break;
              }

          history_length++;
          history_index = history_length - 1;
      }
      
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if ((ch == 8 || ch == 127)&& line_length > 0) {  //this line has modified by myself
      // <backspace> was typed. Remove previous character read.

      //add code by myself here
      int num = position - 1;
      while (num < (line_length - 1)) 
      {
          line_buffer[num] = line_buffer[num + 1];
          write(1, line_buffer[num], 1);
          num++;
      }
      line_buffer[line_length] = ' ';
      line_length--;

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      num = position - 1;
      while (num < line_length)
      {
          ch = line_buffer[num];
          write(1, &ch, 1);
          num++;
      }

      ch = ' ';
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);

      position = position - 1;
      num = line_length;
      while (num > position)
      {
         write(1, &ch, 1);
         num--;
      }
      //add code by myself end

      //original given code doesn't work
/*    // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Remove one character from buffer
      line_length--;
      //minus 1 from position */
    }
    else if (ch == 1)
    {
        //ctrl-d:Home key, the cursor moves to the beginning of the line
        while (position > 0)
        {
            ch = 8;
            write(1, &ch, 1);
            position = position - 1; 
        }

    }
    else if ((line_length > 0) && ch == 4)
    {
        //ctrl-d
        //Removes the character at the cursor
        //It is similar to backspace
        int num = position;
        while (num < (line_length - 1)) 
        {
            line_buffer[num] = line_buffer[num + 1];
            write(1, line_buffer[num], 1);
            num++;
        }
        line_buffer[line_length] = ' ';
        line_length--;

        num = position;
        while (num < line_length)
        {
            ch = line_buffer[num];
            write(1, &ch, 1);
            num++;
        }

        ch = ' ';
        write(1, &ch, 1);
        ch = 8;
        write(1, &ch, 1);
        
        num = line_length;
        while (num > position)
        {
          write(1, &ch, 1);
          num--;
        }
    }
    else if (ch == 5)
    {
        //ctrl-e: End key, the cursor moves to the end of the line
        while(position < line_length)
        {
            ch = line_buffer[position];
            write(1, &ch, 1);
            position = position + 1;
        } 
    }
    else if (ch==27) 
    {
        // Escape sequence. Read two chars more
        //
        // HINT: Use the program "keyboard-example" to
        // see the ascii code for the different chars typed.
        //
        char ch1; 
        char ch2;
        read(0, &ch1, 1);
        read(0, &ch2, 1);
        if ((history_length > 0) && ch1==91 && ch2==65) {
	      // Up arrow. Print last line in history.

	      // Erase old line
	      // Print backspaces
	      int i = 0;
	      for (i =0; i < line_length; i++) 
        {
	          ch = 8;
	          write(1,&ch,1);
	      }

	      // Print spaces on top
	      for (i =0; i < line_length; i++) 
        {
	          ch = ' ';
	          write(1,&ch,1);
	      }

	      // Print backspaces
	      for (i =0; i < line_length; i++) 
        {
	          ch = 8;
	          write(1,&ch,1);
	      }	

	      // Copy line from history
        //the following lines has modified by myself becuase the original given code
        //doesn't work

        int num1 = (history_length + history_index) % history_length;
	      strcpy(line_buffer, history[num1]);
	      line_length = strlen(line_buffer); 
	      history_index = (history_length + history_index - 1) % history_length;

	      // echo line
	      write(1, line_buffer, line_length);
        
        position = line_length;
        arrowNum++;
      }
      else if ((history_length > 0) && ch1 == 91 && ch2 == 66)
      {
          //Done arrow. Print next line in history.
          //very similar to what we have done in up arrow
          // Erase old line
          // Print backspaces
          int i = 0;
          for (i =0; i < line_length; i++) 
          {
              ch = 8;
              write(1,&ch,1);
          }

          // Print spaces on top
          for (i =0; i < line_length; i++) 
          {
              ch = ' ';
              write(1,&ch,1);
          }

          // Print backspaces
          for (i =0; i < line_length; i++) 
          {
              ch = 8;
              write(1,&ch,1);
          } 

          // Copy line from history
          if (arrowNum == 0)
          {
            //first type down arrow
            history_index = history_index - 1;
          }
          int num2 = (history_index + 2) % history_length;
          strcpy(line_buffer, history[num2]);
          line_length = strlen(line_buffer);
          history_index = (history_index + 1) % history_length;
          // echo line
          write(1, line_buffer, line_length);
          
          position = line_length;
          arrowNum++;
      }
      else if (ch1 == 91 && ch2 == 68)
      {
          //left arrow
          if (position > 0)
          {
              ch = 8;
              write(1, &ch, 1);
              position = position - 1;
          }
      }
      else if (ch1 == 91 && ch2 == 67)
      {
        //right error
        if (position < line_length)
        {
            ch = line_buffer[position];
            write(1, &ch, 1);
            position = position + 1;
        }
      }
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  return line_buffer;
}

