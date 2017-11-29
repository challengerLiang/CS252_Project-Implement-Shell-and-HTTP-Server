const char * usage = 
"usage: myhttpd [-f|-t|-p] <port>                 \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

void processOwnRequest(int socket);
void processOwnRequestThread(int socket);
void poolSlave(int socket);

//kill zombie
void killzombie(int signal)
{
    //wait3(0,0,NULL);
    while(waitpid(-1, NULL, WNOHANG) > 0); 
}

int QueueLength = 5;

pthread_mutex_t m1;

int main( int argc, char **argv)
{
  //deal with signal SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  int port;
  int concurrencyChoice = 0;
  // Print usage if not enough arguments
  if (argc == 1) 
  {
      port = 5202;
      concurrencyChoice = 0;
  }
  else if (argc == 2)
  {
      // Get the port from the arguments
      port = atoi(argv[1]);
      concurrencyChoice = 0;
  }
  else if (argc == 3)
  {
      port = atoi(argv[2]);
      //set concurrencyChoice
      if (strcmp(argv[1], "-f") == 0)
      {
          //process based
          concurrencyChoice = 1;
      }
      else if (strcmp(argv[1], "-t") == 0)
      {
          //thread based
          concurrencyChoice = 2;
      }
      else if (strcmp(argv[1], "-p") == 0)
      {
          //pool of thread
          concurrencyChoice = 3;
      }
      else
      {
          fprintf( stderr, "%s", usage );
          exit( -1 );      
      }
  }
  else
  {
      fprintf( stderr, "%s", usage );
      exit( -1 );
  }

   if(port <= 1024 || port >= 65536)
   {
       const char * temp = "the range of port number should be: 1024 < port < 65536\n";
       fprintf(stderr, "%s", temp);
       exit(1);
   }
  
    //zombie elimination
    struct sigaction signalAction;
    signalAction.sa_handler = killzombie;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;

    int zombieError = sigaction(SIGCHLD, &signalAction, NULL );
    if (zombieError)
    {
         perror("sigaction");
         exit(-1);
    }
    //zombie elimination end


  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

  //deal with concurrency
  if (concurrencyChoice == 0 || concurrencyChoice == 1
      ||concurrencyChoice == 2) 
  {
      // "-p" was not be selected
      while ( 1 ) 
      {

          // Accept incoming connections
          struct sockaddr_in clientIPAddress;
          int alen = sizeof( clientIPAddress );
          int slaveSocket = accept( masterSocket,
			        (struct sockaddr *)&clientIPAddress,
			        (socklen_t*)&alen);
     
          //check accpet
          if (slaveSocket == -1 && errno == EINTR)
              continue;
          else if (slaveSocket < 0)
          {
              perror( "accept" );
              exit(-1);
          }


          if (concurrencyChoice == 0)
          {
              // Process request.
              processOwnRequest(slaveSocket);
              // Close socket
              close( slaveSocket );
          }
          else if (concurrencyChoice == 1)
          {
              //process based
              pid_t ret = fork();
              if (ret == 0)
              {
                  processOwnRequest(slaveSocket);
                  close(slaveSocket);
                  exit(EXIT_SUCCESS);    
              }

              close(slaveSocket);
          }
          else if (concurrencyChoice == 2)
          {
              //thread based
              //initialize pthread attributes
              pthread_t t1;
              pthread_attr_t attr;
              pthread_attr_init(&attr);
              pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
              //call pthread create
              pthread_create(&t1, &attr, (void * (*)(void *))processOwnRequestThread, (void *)slaveSocket);

              //pthread_join(t1, NULL);
          }
      }
  }
  else if (concurrencyChoice == 3)
  {
      //initalize mutex;
      pthread_mutex_init(&m1, NULL);
      //initialize pthread attributes
      pthread_t tid[5];
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

      for (int i = 0; i < 5; i++)
          pthread_create(&tid[i], &attr, (void * (*)(void *))poolSlave, (void *)masterSocket);
      
      pthread_join(tid[0], NULL);
      pthread_join(tid[1], NULL);
      pthread_join(tid[2], NULL);
      pthread_join(tid[3], NULL);
      pthread_join(tid[4], NULL);

  }
 
}

   void processOwnRequestThread(int socket)
   {
       processOwnRequest(socket);
       close(socket);
   }

   void poolSlave(int socket)
   {
       while (1)
       {
          struct sockaddr_in clientIPAddress;
          int alen = sizeof( clientIPAddress );

          pthread_mutex_lock(&m1);

          int slaveSocket = accept( socket,
                (struct sockaddr *)&clientIPAddress,
                (socklen_t*)&alen);

          pthread_mutex_unlock(&m1);

          //check if accept worked
          if (slaveSocket < 0)
          {
              perror( "accept" );
              exit( -1 );
          }

          processOwnRequest(slaveSocket);
          close(slaveSocket); 
       } 
   }

   void processOwnRequest(int socket)
   {
        // Buffer used to store the name received from the client
        const int maxLength = 8192;
        char stringFromClient[maxLength + 1];

        int length = 0;
        int n;

        char * docpath = (char *)malloc(maxLength * sizeof(char));
        //initialize docpath
        docpath[0] = '\0';
        int gotGet = 0;
        int gotDotpath = 0;

        //printf("success\n");
/*      const char * prompt = "Connection sccessful\n";
        write(socket, prompt, strlen(prompt));
*/
        // Currently character read
        unsigned char newChar;
        // Last character read
        unsigned char lastChar = 0;

        unsigned char lastLastChar = 0;

        unsigned char lastLastLastChar = 0;

        while (length < maxLength && (n = read(socket, &newChar, sizeof(newChar))) > 0 ) 
        {
            if(newChar == ' ')
            {
                if (gotGet == 0)
                    gotGet = 1;
                else if (gotDotpath == 0)
                {
                    stringFromClient[length] = '\0';
                    strcpy(docpath, stringFromClient);
                    gotDotpath = 1;
                }
            } 
            else if (newChar == '\n' && lastChar == '\r') 
                     //&& lastLastChar == '\n' && lastLastLastChar  == '\r')
                break;
            else
                {
                  lastChar = newChar;
                  if (gotGet == 1)
                  {
                      stringFromClient[length] = newChar;
                      length = length + 1;
                  }
                }                    
        }
        // Read the remaining header and ignore it
        while(n = read(socket, &newChar, sizeof(newChar)) > 0)
        {
            //find the last \r\n\r\n and break
            if (newChar == '\n' && lastChar == '\r' 
                && lastLastChar == '\n' && lastLastLastChar  == '\r')
            break;
            else
            {
                lastLastLastChar = lastLastChar;
                lastLastChar = lastChar;
                lastChar = newChar;
            }     
        }
        stringFromClient[length] = '\0';
        printf("\nstringFromClient = %s\n",stringFromClient);
        //printf("docpath = %s\n",docpath);  

        //map the document path to the real file
        char cwd[512] = {0};
        if  (docpath[0] != '\0')
        {
            getcwd(cwd, sizeof(cwd));
            //printf("cwd = %s\n", cwd);
            if (strcmp(docpath, "/") == 0)
            {
                strcat(cwd, "/http-root-dir/htdocs/index.html");
            }
            else if (strncmp(docpath, "/icons", 6) == 0)
            {
                strcat(cwd, "/http-root-dir/");
                strcat(cwd, docpath);
            }
            else if (strncmp(docpath, "/htdocs", 7) == 0)
            {
                strcat(cwd, "/http-root-dir/");
                strcat(cwd, docpath);
            }
            else
            {
                strcat(cwd, "/http-root-dir/htdocs");
                strcat(cwd, docpath);
            }
        }

        if (strstr(cwd, "..") != NULL)
        {
            //found .. we should expand the ".."
            //str stores the real path
            char str[maxLength + 1] = {0};
            char * indicator = realpath(docpath, str);

            if (indicator != NULL)
                if (strlen(str) > (strlen(cwd) + strlen("/http-root-dir")))
                {
                    //use real path to replace cwd
                    strcpy(cwd, str);
                }
        }

        printf("cwd = %s\n", cwd);

        //determine content type
        char * contentType;
        if (strstr(cwd, ".html") || strstr(cwd, ".html/"))
            contentType = strdup("text/html");
        else if (strstr(cwd, ".gif") || strstr(cwd, ".gif/"))
            contentType = strdup("image/gif");
        else
            contentType = strdup("text/plain");

        //open file
        FILE * file;
        file = fopen(cwd, "r");
        /*if (file != NULL)
        {
          printf("file != NULL\n");
        }
*/
        const char * clrf = "\r\n";
        if (file != NULL)
        {
            write(socket, "HTTP/1.0", strlen("HTTP/1.0"));
            write(socket, " ", 1);
            write(socket, "200", 3);
            write(socket, " ", 1);
            write(socket, "Document", strlen("Document"));
            write(socket, " ", 1);
            write(socket, "follows", strlen("follows"));
            write(socket, clrf, 2);
            write(socket, "Server:", strlen("Server:"));
            write(socket, " ", 1);
            //server type is "CS 252 lab 5"
            write(socket, "CS 252 lab5", strlen("CS 252 lab5"));
            write(socket, clrf, 2);

            write(socket, "Content-type:", strlen("Content-type:"));
            write(socket, " ", 1);
            write(socket, contentType, strlen(contentType));
            write(socket, clrf, 2);
            write(socket, clrf, 2);
            //document data
            int sign = 0;
            char chr;
            while(sign = read(fileno(file), &chr, sizeof(chr)))
                if (write(socket, &chr, sizeof(chr)) != sign)
                {
                    perror("write");
                    return;
                }
            //close file
            fclose(file);
        }
        else
        {
            //send error message
            const char *notFound = "404 Not Found"; 
            write(socket, "HTTP/1.0", strlen("HTTP/1.0"));
            write(socket, " ", 1);
            write(socket, notFound, strlen(notFound));
            write(socket, clrf, 2);
            write(socket, "Server:", sizeof("Server:"));
            write(socket, " ", 1);
            write(socket, "CS 252 lab5", strlen("CS 252 lab5"));
            write(socket, clrf, 2);
            write(socket, "Content-type:", strlen("Content-type:"));
            write(socket, " ", 1);
            write(socket, contentType, strlen(contentType));
            write(socket, clrf, 2); 
            write(socket, clrf, 2); 
            write(socket, notFound, strlen(notFound));
            write(socket, clrf, 2); 
        }
    }
