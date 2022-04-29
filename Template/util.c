#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "util.h"

//Global variables
static int master_fd = -1; // Global var used to store socket


/**********************************************
 * makeargv
 * @param s is the buffer that needs to be parsed.
 * @param delimeters is used to split the buffer. Will be used by strtok_r 
 * @param argvp is adress to string array (so charr*** type) -> Try to reason why this needs to be char** type
 * 
 * Returns number of tokens on sucess and sets argvp -1 otherwise
 * Usage example: 
 *    char* buf = "This is the string I want to parse \n";
 *    char **result;
 *    makeargv(buf," \n",&result)
 *    result contains => ["This","is","the","string","I","want","to","parse"]
************************************************/
int makeargv(const char *s, const char *delimiters, char ***argvp) {
   int error;
   int i;
   int numtokens;
   const char *snew;
   char *t;
   char* context;

   if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) {
      errno = EINVAL;
      return -1;
   }

   *argvp = NULL;
   snew = s + strspn(s, delimiters);

   if ((t = malloc(strlen(snew) + 1)) == NULL)
      return -1;
   strcpy(t,snew);
   
   numtokens = 0;
   if (strtok_r(t, delimiters,&context) != NULL)
      for (numtokens = 1; strtok_r(NULL, delimiters,&context) != NULL; numtokens++) ;
   
   
   if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL) {
      error = errno;
      free(t);
      errno = error;
      return -1;
   }

   if (numtokens == 0)
      free(t);
   else {
      strcpy(t,snew);
      **argvp = strtok_r(t,delimiters,&context);
      for (i=1; i<numtokens; i++){
         *((*argvp) +i) = strtok_r(NULL,delimiters,&context);
      }
         
   }
   *((*argvp) + numtokens) = NULL;

   return numtokens;
}

void freemakeargv(char **argv) {
   if (argv == NULL)
      return;
   if (*argv != NULL)
      free(*argv);
   free(argv);
}

/**********************************************
 * init
   - port is the number of the port you want the server to be
     started on
   - initializes the connection acception/handling system
   - YOU MUST CALL THIS EXACTLY ONCE (not once per thread,
     but exactly one time, in the main thread of your program)
     BEFORE USING ANY OF THE FUNCTIONS BELOW
   - if init encounters any errors, it will call exit().
************************************************/
void init(int port) {
	// set up sockets using bind, listen
	// also do setsockopt(SO_REUSEADDR)
	// exit if the port number is already in use
	
	//Create socket, store return value in master_fd
	master_fd = -1;
	master_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(master_fd == -1) {
		printf("Error, could not create socket. Exiting.\n");
		exit(-1);
	} else {	
		printf("Socket created (master_fd: %d)\n", master_fd);
	}	

	//Use setsockopt with SO_REUSEADDR
	int enable = 1, sockopt_return_code;
	sockopt_return_code = setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(int));

	if(sockopt_return_code == -1) {	
		printf("Error, could not set socket options. Exiting.\n"); 
		exit(-1);
	} else {	
		printf("Socket options set\n");	
	}

	//Bind socket
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port); // Server picks port
	if(bind(master_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {	
		printf("Error, could not bind socket. Exiting.\n"); 
		exit(-1);
	} else {
		printf("Socket bound\n");	
	}

	//Listen
 	if(listen(master_fd, 20) == -1) {
		printf("Error, could not listen on socket. Exiting.\n"); 
		exit(-1);
	} else {
		printf("Socket listening\n");	
	}
   
	//If successful should print
	printf("UTILS.O: Server Started on Port %d\n",port);

}

/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
     DO NOT use the file descriptor of your own -- use
     get_request() instead.
   - if the return value is negative, the thread calling
     accept_connection must should ignore request.
***********************************************/
int accept_connection(void) {
	
	struct sockaddr_in client_addr;
	int client_fd, addr_size = sizeof(client_addr);
	
	// Accept the connection
	client_fd = accept(master_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_size);
	
	// Returns fd on success, -1 on failure.
	return client_fd;
}

/**********************************************
 * get_request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        from where you wish to get a request
      - filename is the location of a character buffer in which
        this function should store the requested filename. (Buffer
        should be of size 1024 bytes.)
   - returns 0 on success, nonzero on failure. You must account
     for failures because some connections might send faulty
     requests. This is a recoverable error - you must not exit
     inside the thread that called get_request. After an error, you
     must NOT use a return_request or return_error function for that
     specific 'connection'.
************************************************/
int get_request(int fd, char *filename) {
   /*
      Read from the socketfd and parse the first line for the GET info
      Error checks: 
         - if it isn't a GET request
         - if the file starts with .. or // -> Security breach (Accessing file outside server roor dir)
         - if the file path length is greater than 1024 (Possible buffer overflow)
         In all the cases just dump it and return -1.
      
      otherwise, store the path/filename into 'filename'.
   */

	//Read first line from fd (The rest does not matter to us)
	//Look at fgets
	
	char buf[1024];
	int bytes_read;
	if((bytes_read = read(fd, buf, 1024)) == -1) {
		printf("Error, could not read fd\n");
		return -1;
	}

	if(strchr(buf, '\0') == NULL) {
		printf("Error, possible buffer overflow.\n");
		return -1;
	}


	//Pass the first line read into makeargv
	//Look at makeargv comments for usage
	char ** result;
 	makeargv(buf," \n",&result);
	
	//Error checks	
	if(strstr(result[0], "GET") == NULL) {
		printf("Bad GET request.\n");
		return -1;
	}

	if(strstr(result[1], "..") != NULL || strstr(result[1], "//") != NULL) {
		printf("Malicious file requested.\n");
		return -1;	
	}

	if(strlen(result[1]) > 1024) {
		printf("Possible overflow.\n");
		return -1;
	}

	//Passed error checks, copy the path into filename
	strcpy(filename, result[1]);
	printf("File requested: %s\n", filename);
	
	//Call freemakeargv to free memory
	freemakeargv(result);

	//Return success
	return 0;

}

/**********************************************
 * return_result
   - returns the contents of a file to the requesting client
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the result of a request
      - content_type is a pointer to a string that indicates the
        type of content being returned. possible types include
        "text/html", "text/plain", "image/gif", "image/jpeg" cor-
        responding to .html, .txt, .gif, .jpg files.
      - buf is a pointer to a memory location where the requested
        file has been read into memory (the heap). return_result
        will use this memory location to return the result to the
        user. (remember to use -D_REENTRANT for CFLAGS.) you may
        safely deallocate the memory after the call to
        return_result (if it will not be cached).
      - numbytes is the number of bytes the file takes up in buf
   - returns 0 on success, nonzero on failure.
************************************************/
int return_result(int fd, char *content_type, char *buf, int numbytes) {
   /*
      send headers back to the socketfd, connection: close, content-type, content-length, etc
      then finally send back the resulting file
      then close the connection.

      It is IMPORTANT that you follow the format as described in the writeup while writing to the socket.

   */

	char httpOK[17], conType[26], conLen[32], connection[19], blankLine[2];
	sprintf(httpOK, "HTTP/1.1 200 OK\n");
	sprintf(conType, "Content-Type: %s\n", content_type);
	sprintf(conLen, "Content-Length: %d\n", numbytes);
	sprintf(connection, "Connection: Close\n");
	sprintf(blankLine, "\n");

	int returnVal = 0, bytesWritten = 0;

	//Send headers
	if((bytesWritten = write(fd, httpOK, 17)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}


	if((bytesWritten = write(fd, conType, 26)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	if((bytesWritten = write(fd, conLen, 32)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	if((bytesWritten = write(fd, connection, 19)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	if((bytesWritten = write(fd, blankLine, 2)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	//Send Result file
	if((bytesWritten = write(fd, buf, numbytes)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	//Close connection
	if(close(fd) == -1) {
		printf("Error, unable to close fd %d\n", fd);
	}
	
	//Return success
	return returnVal;
}


/**********************************************
 * return_error
   - returns an error message in response to a bad request
   - parameters:
      - fd is the file descriptor obtained by accept_connection()
        to where you wish to return the error
      - buf is a pointer to the location of the error text
   - returns 0 on success, nonzero on failure.
************************************************/
int return_error(int fd, char *buf) {
   /*
      send 404 headers back to the socketfd, connection: close, content-type: text/plain, content-length
      send back the error message as a piece of text.
      then close the connection

      It is IMPORTANT that you follow the format as described in the writeup while writing to the socket.
   */
   
   //Send headers
   	int returnVal = 0, numbytes = 25;
	int bytesWritten = 0; 
	char httpError[24], conType[26], conLen[32], connection[19], blankLine[2];
	sprintf(httpError, "HTTP/1.1 404 Not Found\n");
	sprintf(conType, "Content-Type: text/html\n");
	sprintf(conLen, "Content-Length: %d\n", numbytes);
	sprintf(connection, "Connection: Close\n");
	sprintf(blankLine, "\n");

	
	buf = "Requested file not found.\n";

	//Send headers
	if((bytesWritten = write(fd, httpError, 24)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}


	if((bytesWritten = write(fd, conType, 26)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	if((bytesWritten = write(fd, conLen, 32)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	if((bytesWritten = write(fd, connection, 19)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	if((bytesWritten = write(fd, blankLine, 2)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	}

	//Send Result file
	if((bytesWritten = write(fd, buf, numbytes)) <= 0) {
		printf("Error, unable to write to fd %d\n", fd);
		returnVal = -1;
	} 

	//Close connection
	if(close(fd) == -1) {
		printf("Error, unable to close fd %d\n", fd);
	}
	
	//Return success
	return returnVal;
}
