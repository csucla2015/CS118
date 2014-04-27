/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <fcntl.h>
#include <time.h> 
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}
/* This is what we will print to the console if the request is successful  */

static char* success_string =
    "HTTP/1.1 200 OK\n"
    "Content-type: %s\n"
    "\n";

/* HTTP response, header, and body indicating that the we didn't
   understand the request.  */

static char* unsuccessful_string = 
    "HTTP/1.1 400 Bad Request\n"
    "Content-type: text/html\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Bad Request</h1>\n"
    "  <p>This server did not understand your request.</p>\n"
    " </body>\n"
    "</html>\n";

/* HTTP response, header, and body template indicating that the
   requested document was not found.  */

static char* notfound_string = 
    "HTTP/1.1 404 Not Found\n"
    "Content-type: text/html\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Sorry</h1>\n"
    "  <p>We could not find this file on zero. Apologies.</p>\n"
    " </body>\n"
    "</html>\n";

/* HTTP response, header, and body template indicating that the
   method was not understood.  */

static char* unknownrequest_string = 
    "HTTP/1.1 501 Method Not Implemented\n"
    "Content-type: text/html\n"
    "\n"
    "<html>\n"
    " <body>\n"
    "  <h1>Method Not Implemented</h1>\n"
    "  <p>The method %s is not implemented by this server.</p>\n"
    " </body>\n"
    "</html>\n";

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}
static void get_resource (int newsockfd, char* url) {
    int resourcefd, i;
    char c, *file_type, *header_type;
    char response[1024];

    printf("Retrieving resource %s\n", url);

    // Try to open the file. If we can't open the file, tell the client we can't find it and log the error to stderr.
    resourcefd = open(++url, O_RDONLY);
    if (resourcefd < 0) {
        snprintf (response, sizeof (response), notfound_string, url);
        write (newsockfd, response, strlen (response));
        error ("ERROR opening file");
    }
    
    // Get the file extension. If there isn't one explicitly stated, then assume HTML
    file_type = strrchr (url, '.');
    if (!file_type)
        file_type = "html";
    else
        file_type++;

    // Match the header type with the file type in generating the response. Only support JPEG, GIF, and HTML files for now.
    if (strcasecmp (file_type, "jpg") == 0 || strcasecmp (file_type, "jpeg") == 0)
        header_type = "image/jpeg";
    else if (strcasecmp (file_type, "gif") == 0)
        header_type = "image/gif";
    else
        header_type = "text/html";

    // Begin to write the response to the client: OK, file type
    snprintf(response, sizeof (response), success_string, header_type);
    write(newsockfd, response, strlen (response));
    while ( (i = read(resourcefd, &c, 1)) ) {
        if ( i < 0 )
            error("ERROR reading from file.");
        if ( write(newsockfd, &c, 1) < 1 )
            error("ERROR sending file.");
    }
}

static void handle_request (int newsockfd)
{
    char buffer[512];
    ssize_t bytes_read;

    /* Read some data from the client.  */
    bytes_read = read (newsockfd, buffer, sizeof (buffer) - 1);
    if (bytes_read > 0) {
        char method[sizeof (buffer)];
        char url[sizeof (buffer)];
        char protocol[sizeof (buffer)];

        /* Some data was read successfully.  NUL-terminate the buffer so
        we can use string operations on it.  */
        buffer[bytes_read] = '\0';
        /* The first line the client sends is the HTTP request, which is
        composed of a method, the requested page, and the protocol
        version.  */
        sscanf (buffer, "%s %s %s", method, url, protocol);
        printf("Received request:\n%s", buffer);

        /* Check the protocol field.  We understand HTTP versions 1.0 and
        1.1.  */
        if (strcmp (protocol, "HTTP/1.0") && strcmp (protocol, "HTTP/1.1")) {
            /* We don't understand this protocol.  Report a bad response.  */
            write (newsockfd, unsuccessful_string, strlen (unsuccessful_string));
        }
        else if (strcmp (method, "GET")) {
            /* This server only implements the GET method.  The client
            specified some other method, so report the failure.  */
            char response[1024];

            snprintf (response, sizeof (response), unknownrequest_string, method);
            write (newsockfd, response, strlen (response));
        }
        else 
            /* A valid request.  Process it.  */
            get_resource (newsockfd, url);
    }
    else if (bytes_read == 0)
        /* The client closed the connection before sending any data.
        Nothing to do.  */
        ;
    else 
        /* The call to read failed.  */
        error("ERROR reading from request");
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

	char buffer[1025];
    int n; // I may have to make this int, check it out clilen;
    time_t ticks; 

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
    memset(buffer, '0', sizeof(buffer)); 
     portno = 5000;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,10);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
	/*
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
*/
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             handle_request(newsockfd);
             if (close(newsockfd) < 0)
                error("ERROR closing connection");
            // end forked process
            exit(0);
         }
         else //returns the process ID of the child process to the parent
	{
	    if (close(newsockfd) < 0)
                error("ERROR closing connection in parent");
            waitpid(-1, NULL, WNOHANG);
	} 
     } /* end of while */
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n;
   char buffer[256];
      
   bzero(buffer,256);
   n = read(sock,buffer,255);
   if (n < 0) error("ERROR reading from socket");
   printf("Here is the message: %s\n",buffer);
   n = write(sock,"I got your message",18);
   if (n < 0) error("ERROR writing to socket");
}
