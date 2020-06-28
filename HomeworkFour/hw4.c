/******************************************************************

hw4.c
This program is an implementation of a multi-threaded chat server
using sockets. This allows clients to send and recive short
text messages.

A lot of this code was "taken" from the notes on udp and tcp
in lecture.

Authors: Daniel Dukeshire
Date 4.23.2020

******************************************************************/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXBUFFER 3000
#define MAXCLIENTS 32
struct client** clients;
int current_clients;

/******************************************************************
  Used in qsort to sort an array of strings
******************************************************************/
int compare(const void* char1, const void* char2){ return strcmp(*(const char**)char1, *(const char**) char2); }

/******************************************************************
client is used to represent a tcp connection. This is used
in threading.
******************************************************************/
struct client
{
  int status;                                                               // The Login Status of the client:
                                                                            // 0 for logged out
                                                                            // 1 for logged in
  int file_descriptor;                                                      // Used for send
  char* username;                                                           // The username of the client
  int loc;                                                                  // The location in the array
};

/******************************************************************
Creates a dynamically allocated array of client structs. This
is used when threading
******************************************************************/
struct client** createClients()
{
  struct client** temp_array = calloc(MAXCLIENTS, sizeof(struct client*));  // A pointer array of client pointers
  for(int i=0; i<MAXCLIENTS; i+=1)                                          // Initialize all of the values
  {
    struct client* temp_client = calloc(1, sizeof(struct client));          // Initialize a single client to be added
    temp_client->status = 0;                                                // currentlt
    temp_client->username = calloc(17, sizeof(char));                           // The maximum username size is 16 (4-16)
    strcpy(temp_client->username, "EMPTY");                                 // Starts empty
    temp_client->file_descriptor = 0;                                       // Starts with no fd
    temp_client->loc = -1;                                                  // No location

    temp_array[i] = temp_client;                                            // Assign to the array
  }

  return temp_array;
}

/******************************************************************
Frees everything in the clients array
******************************************************************/
void freeClients()
{
  if(clients == NULL) return;                                             // No list, return
  else
  {
    for(int i=0; i<MAXCLIENTS; i++)                                       // Loops across the array
    {
      free(clients[i]->username);                                         // Frees the username
      free(clients[i]);                                                   // Frees the client itself
    }
    free(clients);                                                        // Frees the entire array
  }
}

/******************************************************************
Counts the number of commands inputted from calloced array
******************************************************************/
int countCommands(char** commands)
{
  int i = 0;
  while(commands[i]!=NULL) i+=1;
  return i;
}

/******************************************************************
Turns an input string into an array of commands
******************************************************************/
char ** getCommands(int n, char buffer[n])
{
  char* token1 = strtok(buffer, "\n");                                    // Gets everything up until the first newline
  char* token2 = strtok(NULL, "\n");                                      // Gets eveything up until the second newline

  char* token3 = strtok(token1, " ");

  char** commands = calloc(8, sizeof(char*));                             // Allocate space for the commands array
  int current = 0;
  while(token3 != NULL)                                                   // Splits the first newline string on all spaces
  {
    commands[current] = token3;
    current +=1;
    token3 = strtok(NULL, " ");
  }
  commands[current] = token2;                                             // Sets the last value properly

  return commands;
}

/******************************************************************
Creates a new TCP connection, called when a thread is spawned
******************************************************************/
void* newTcpConnection(void* arg)
{
  char buffer[MAXBUFFER];                                                 // A buffer to read in from the client
  struct client* client = arg;                                            // Take the input from the thread
  int n;                                                                  // The number of bytes to be read in
                                                                          // TODO: INVALID ___ FORMAT
  do
  {
    n = recv(client->file_descriptor, buffer, MAXBUFFER, 0);
    if(n == -1)                                                           // Check to see if the recieve works properly
    {
      fprintf(stderr, "CHILD %ld: ERROR recv() failed\n", pthread_self());
      break;
    }
    else if( n == 0) continue;                                            // If nothing was read in, go on to the next input
    buffer[n] = '\0';                                                     // must null-terminate the string
    char** commands = getCommands(n, buffer);                             // Gets the commands from input from TCP
    int num_commands = countCommands(commands);                           // The number of commands inputted

    if(commands[0] == NULL)                                               // If I dont recieve anything...
    {
      send(client->file_descriptor, "ERROR Invalid input format\n", 27, 0);
      printf("CHILD %ld: Sent ERROR (Invalid input format)\n", pthread_self());
    }
    else if(strcmp("LOGIN", commands[0]) == 0)                            // Check for LOGIN Request ------------------------------
    {
      printf("CHILD %ld: Rcvd LOGIN request for userid %s\n", pthread_self(), commands[1]);
      if(num_commands != 2)                                               // Proper Login Arguments
      {
        printf("CHILD %ld: Sent ERROR (Wrong number of arguments)\n", pthread_self());
        send(client->file_descriptor, "ERROR Invalid LOGIN format\n", 27, 0);
        continue;
      }

      int is_alnum = 1;             // Determine if the entire string is alphanumeric
      int is_used = 1;              // Determine if the username is already in use
      for (int i = 0; i < strlen(commands[1]); i++) if(!isalnum(commands[1][i])) { is_alnum = 0; break; }
      for(int i=0; i<MAXCLIENTS; i++) if(strcmp(commands[1], clients[i]->username) == 0) { is_used = 0; break; }

      if ( is_used == 0 || client->status == 1)            // If the username is already in use OR client->status == 1 ??
      {
        send(client->file_descriptor, "ERROR Already connected\n", 24, 0);
        printf("CHILD %ld: Sent ERROR (Already connected)\n", pthread_self());
        continue;
      }
      if( is_alnum == 0 || strlen(commands[1]) <= 3 || strlen(commands[1]) > 16 ) // If the string is not alphanumeric
      {
        send(client->file_descriptor, "ERROR Invalid userid\n", 21, 0);
        printf("CHILD %ld: Sent ERROR (Invalid userid)\n", pthread_self());
        continue;
      }
      send(client->file_descriptor, "OK!\n", 4, 0);                       // If passes all tests, then we ok!
      client->status = 1;                                                 // Update logged-in status
      strcpy(client->username, commands[1]);                              // Update the username
    }
    else if(strcmp(commands[0], "WHO") == 0 )                             // Check for a WHO Request -----------------------------
    {
      printf("CHILD %ld: Rcvd WHO request\n", pthread_self());            // Sends to the user
      if(num_commands != 1)
      {
        printf("CHILD %ld: Sent ERROR (Wrong number of arguments)\n", pthread_self());
        send(client->file_descriptor, "ERROR Invalid WHO format\n", 25, 0);
        continue;
      }

      int loc = 0;                                                        // the current location of the logged_in_clients variable
      char** logged_in_clients = calloc(MAXCLIENTS, sizeof(char*));       // A 2D array representing the clients that are logged in
      bool flag = false;

      for(int i = 0; i < MAXCLIENTS; i++) { if(clients[i]->status == 1){ flag = true; logged_in_clients[loc] = clients[i]->username; loc++;} }
      if(flag == false)
      {
        send(client->file_descriptor, "OK!\n", 4, 0);                     // Checking to see if there even is a value to be sent
        continue;
      }
      else
      {
        qsort(logged_in_clients, loc, sizeof(char**), compare);           // Sort the array by ascii value

        char* message = calloc(MAXBUFFER, sizeof(char));                  // Create a string to send a message
        strcpy(message,"OK!\n");                                          // Begin with OK!

        for(int i=0; i<loc; i++)                                          // Adding all the values from the sorted array
        {
          strcat(message, logged_in_clients[i]);                          // Each individual value ...
          strcat(message, "\n");
        }
        int len = strlen(message);
        send(client->file_descriptor, message, len, 0);                   // Send it to the client
        free(message);
      }
    }
    else if(strcmp("LOGOUT", commands[0]) == 0 && client->status == 1)    // Check for LOGOUT Request ------------------------------
    {
      printf("CHILD %ld: Rcvd LOGOUT request\n", pthread_self());
      if(num_commands != 1)
      {
        printf("CHILD %ld: Sent ERROR (Wrong number of arguments)\n", pthread_self());
        send(client->file_descriptor, "ERROR Invalid LOGOUT format\n", 28, 0);
        continue;
      }
      send(client->file_descriptor, "OK!\n", 4, 0);
      //sleep(1);
      strcpy(client->username, "EMPTY");
      client->status = 0;                                                 // Simply reset the status
    }
    else if(strcmp("SEND", commands[0]) == 0 && client->status == 1)      // Check for SEND Request --------------------------------
    {
      printf("CHILD %ld: Rcvd SEND request to userid %s\n", pthread_self(), commands[1]);
      if(num_commands != 4)
      {
        printf("CHILD %ld: Sent ERROR (Wrong number of arguments)\n", pthread_self());
        send(client->file_descriptor, "ERROR Invalid SEND format\n", 26, 0);
        continue;
      }
      int message_length = atoi(commands[2]);
      if(message_length <= 0 || message_length >= 991)                    // Have to check to see if the message length works
      {
        send(client->file_descriptor, "ERROR Invalid msglen\n", 21, 0);
        printf("CHILD %ld: Sent ERROR (Invalid msglen)\n", pthread_self());
        continue;
      }
      bool is_valid = false;                                              // Loop over clients, find the person to send to
      int client_fd = 0;
      for(int i=0; i<MAXCLIENTS; i++) { if(strcmp(commands[1], clients[i]->username) == 0) { is_valid= true; client_fd = clients[i]->file_descriptor; break;}}
      if(is_valid == false)
      {
        send(client->file_descriptor, "ERROR Unknown userid\n", 21, 0);   // Here, could not find the user
        printf("CHILD %ld: Sent ERROR (Unknown userid)\n", pthread_self());
        continue;
      }
      else
      {
        send(client->file_descriptor, "OK!\n", 4, 0);
        sleep(1);                                                         // ensures read properly

        char * message = calloc(MAXBUFFER, sizeof(char));                 // Preparing the message
        strcpy(message, "FROM ");                                         // FROM
        strcat(message, client->username);                                // USERNAME
        strcat(message, " ");                                             //
        strcat(message, commands[2]);                                     // (length of message)
        strcat(message, " ");                                             //
        strcat(message, commands[3]);                                     // MESSAGE
        strcat(message, "\n");
        int len = strlen(message);
        send(client_fd, message, len, 0);                                 // Sends to the proper user
        free(message);
      }
    }
    else if(strcmp("BROADCAST", commands[0]) == 0 && client->status == 1) // Check for BROADCAST Request --------------------------------
    {
      printf("CHILD %ld: Rcvd BROADCAST request\n", pthread_self());
      if(num_commands != 3)
      {
        printf("CHILD %ld: Sent ERROR (Wrong number of arguments)\n", pthread_self());
        send(client->file_descriptor, "ERROR Invalid BROADCAST format\n", 31, 0);
        continue;
      }
      int message_length = atoi(commands[1]);
      if(message_length <= 0  || message_length >= 991)                   // Check to see if the message is of the right size
      {
        send(client -> file_descriptor, "ERROR Invalid msglen\n", 21, 0);
        printf("CHILD %ld: Sent ERROR (Invalid msglen)\n", pthread_self());
        continue;
      }
      send(client->file_descriptor, "OK!\n", 4, 0);
      sleep(1);

      char * message = calloc(MAXBUFFER, sizeof(char));
      strcpy(message, "FROM ");                                           // FROM
      strcat(message, client->username);                                  // USERNAME
      strcat(message, " ");                                               //
      strcat(message, commands[1]);                                       // (length of message)
      strcat(message, " ");                                               //
      strcat(message, commands[2]);                                       // MESSAGE
      strcat(message, "\n");
      int len = strlen(message);
      for(int i=0; i<MAXCLIENTS; i++){ if(clients[i]->status != 0){ send(clients[i]->file_descriptor, message, len, 0);}}
    }
    else                                                                // Otherwise, it is the wrong request !
    {
      printf("CHILD %ld: Sent ERROR (Command not supported over TCP)\n", pthread_self());
      char* message = calloc(MAXBUFFER, sizeof(char*));
      strcpy(message, "ERROR ");
      strcat(message, commands[0]);
      strcat(message, " not supported over TCP\n");
      send(client->file_descriptor, message, strlen(message), 0);
      free(message);
    }
  } while(n > 0);

  printf("CHILD %ld: Client disconnected\n", pthread_self());             // Disconnected !
  strcpy(client->username, "EMPTY");                                      // So.. we reset everything !!
  client->status = 0;
  client->file_descriptor = 0;
  current_clients-=1;
  pthread_exit(NULL);                                                     // Exit the thread
}

/******************************************************************
 main() - Sets up the TCP and UDP connections, performs UDP actions.
******************************************************************/
int main(int argc, char* argv[])
{
  setvbuf(stdout, NULL, _IONBF, 0);
  if(argc == 1)                                                           // Ensuring proper number of arguments
  {
    fprintf(stderr, "ERROR incorrect number of arguments");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server;                                              // Declaring server struct
  struct sockaddr_in client;                                              // Declaring client struct
  int port = atoi(argv[1]);                                               // Getting the port value from the console
  server.sin_family = PF_INET;                                            // AF_INET ???
  server.sin_addr.s_addr = htonl(INADDR_ANY);                             // Allows any IP address to connect
  server.sin_port = htons(port);                                          // Data Marshalling
  int server_len = sizeof(server);                                        // The size of the server
  int client_len = sizeof(client);                                        // The size of the client
  char buffer[MAXBUFFER];                                                 // Buffer to be read in by UDP
  fd_set fd_table;                                                        // Declaring  the fd table
  current_clients = 0;                                                    // The total number of clients in the server at one time
  clients = createClients();                                              // Creates all of the client values
  pthread_t tid;                                                          // the pid for threading

  printf("MAIN: Started server\n");

  // SETTING UP the TCP CONNECTION ----------------------------------------------------------------------
  int fd_tcp = socket( PF_INET, SOCK_STREAM, 0 );
  if(fd_tcp == -1)                                                         // Creating Socket for the TCP
  {
    perror( "ERROR TCP socket() failed" );
    return EXIT_FAILURE;
  }
  if(bind(fd_tcp, (struct sockaddr *)&server, sizeof(server))<0)           // Attempting to bind
  {
    perror( "ERROR TCP bind() failed" );
    return EXIT_FAILURE;
  }
  if(listen(fd_tcp, 5)< 0 )                                                // Attemptinf to listen
  {
    perror( "ERROR TCP listen() failed" );
    return EXIT_FAILURE;
  }
  else printf( "MAIN: Listening for TCP connections on port: %d\n", port);    // Printing to console

  // SETTING UP the UDP Connection ----------------------------------------------------------------------
  int fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
  if (bind(fd_udp,(struct sockaddr *)&server,sizeof(server)) < 0)         // Attempting to bind
  {
      perror( "ERROR UDP bind() failed\n" );
      return EXIT_FAILURE;
  }
  if ( getsockname( fd_udp, (struct sockaddr *) &server, (socklen_t *) &server_len ) < 0 ){
      perror( "getsockname() failed for UDP" );
      return EXIT_FAILURE;
  }
  else printf( "MAIN: Listening for UDP datagrams on port: %d\n", port ); // Printing to console

  // Dealing with file descriptors
  FD_ZERO(&fd_table);                                                     // Zeroing out the fd table
  int fd_max;                                                             // Finding the max fd value
  if(fd_udp > fd_tcp) fd_max = fd_udp + 1;
  else fd_max = fd_tcp + 1;

  // Starting the application for waiting on TCP and UDP calls ------------------------------------------
  while(true)
  {
    FD_SET(fd_tcp, &fd_table);
    FD_SET(fd_udp, &fd_table);
    int tcp_v_udp = select(fd_max, &fd_table, NULL, NULL, NULL);

    if(tcp_v_udp == 0)                                                  // Called on failure of select
    {
      perror("ERROR No connection\n");
      continue;
    }
    if(FD_ISSET(fd_tcp, &fd_table))                                     // RECIEVING INPUT FROM TCP -----------------------------------
    {
      int fd_new = accept(fd_tcp, (struct sockaddr *)& client, (socklen_t *)& client_len);
      if(fd_new == -1)                                                  // Error Check for the accept call
      {
        fprintf(stderr, "ERROR accept() failed\n");
        return EXIT_FAILURE;
      }
      if(current_clients == MAXCLIENTS)                                 // Check to see if there is room to add the next client
      {
        send(fd_new, "ERROR server is full\n", 21, 0);
        printf("MAIN: SEND ERROR (server is full)\n");
        continue;
      }
      else printf("MAIN: Rcvd incoming TCP connection from: %s\n", inet_ntoa(client.sin_addr));

      int first_open = 0;                                               // Loop to find the first open location
      for(int i=0; i<MAXCLIENTS; i++)                                   // in the clients array
      {
        if(clients[i]->file_descriptor == 0)                            // If I find a open location
        {
          first_open = i;                                               // Save the location
          break;                                                        // break the loop
        }
      }
      clients[first_open]->file_descriptor = fd_new;                    // Set the new tcp client variables
      clients[first_open]->loc = first_open;                            // Assigns the location in the array
      current_clients +=1;                                              // Increment the number of clients in the clients array
      int r = pthread_create(&tid, NULL, newTcpConnection, clients[first_open]);          // Spawn off a child thread
      if (r != 0)                                                       // An error check
      {
          fprintf(stderr, "ERROR Could not create thread\n");
          return EXIT_FAILURE;
      }
    }
    if(FD_ISSET(fd_udp, &fd_table))                                   // Accepting UDP Client ----------------------------------------
    {
      int n = recvfrom(fd_udp, buffer, sizeof(buffer), 0, (struct sockaddr *)& client, (socklen_t *)& client_len);

      if(n == -1)                                                     // Check to see if the recvfrom failed
      {
        fprintf(stderr, "ERROR UDP recvfrom() failed\n");
      }
      else
      {
        printf("MAIN: Rcvd incoming UDP datagram from: %s\n", inet_ntoa(client.sin_addr));
        buffer[n] = '\0';
        char** commands = getCommands(n, buffer);

        if(commands[0] == NULL)                                               // If I dont recieve anything...
        {
          printf("MAIN: Sent ERROR (Wrong number of arguments)\n");
          sendto(fd_udp, "ERROR Invalid input format\n", 27, 0, (struct sockaddr*)&client, client_len);
        }
        else if(strcmp(commands[0], "WHO") == 0)                              // Check for a WHO Request -----------------------------
        {
          int loc = 0;                                                        // the current location of the logged_in_clients variable
          char** logged_in_clients = calloc(MAXCLIENTS, sizeof(char*));       // A 2D array representing the clients that are logged in
          bool flag = false;

          for(int i = 0; i < MAXCLIENTS; i++) { if(clients[i]->status == 1){ flag = true; logged_in_clients[loc] = clients[i]->username; loc++;} }
          if(flag == false)
          {
            sendto(fd_udp, "OK!\n", 4, 0,(struct sockaddr*)& client, client_len);                                   // Checking to see if there even is a value to be sent
            continue;
          }
          else
          {
            qsort(logged_in_clients, loc, sizeof(char**), compare);           // Sort the array by ascii value

            char* message = calloc(MAXBUFFER, sizeof(char));                  // Create a string to send a message
            strcpy(message,"OK!\n");                                          // Begin with OK!

            for(int i=0; i<loc; i++)                                          // Adding all the values from the sorted array
            {
              strcat(message, logged_in_clients[i]);                          // Each individual value ...
              strcat(message, "\n");
            }
            int len = strlen(message);
            sendto(fd_udp, message, len, 0, (struct sockaddr*)&client, client_len);
            free(message);
          }
        }
        else if(strcmp("BROADCAST", commands[0]) == 0)                        // Check for BROADCAST Request -----------------------------
        {
          printf("MAIN: Rcvd BROADCAST request\n");
          int message_length = atoi(commands[1]);
          if(message_length <= 0  || message_length >= 991)                   // Check to see if the message is of the right size
          {
            printf("MAIN: Sent ERROR (Invalid msglen)\n");
            sendto(fd_udp, "ERROR Invalid msglen\n",21,0, (struct sockaddr*)& client, client_len);
            continue;
          }
          sendto(fd_udp, "OK!\n", 4, 0,(struct sockaddr*)& client, client_len);
          sleep(1);

          char * message = calloc(MAXBUFFER, sizeof(char));
          strcpy(message, "FROM UDP-client ");                                  // FROM UDP CLIENT
          strcat(message, commands[1]);                                         // (length of message)
          strcat(message, " ");                                                 //
          strcat(message, commands[2]);                                         // MESSAGE
          strcat(message, "\n");
          int len = strlen(message);
          for(int i=0; i<MAXCLIENTS; i++){ if(clients[i]->status != 0){
            sendto(clients[i] -> file_descriptor, message, len,0,(struct sockaddr*)& client, client_len);}}
        }
        else                                                                    // Otherwise .... unknown command !!
        {
          printf("MAIN: Sent ERROR (not supported over UDP)\n");
          char* message = calloc(MAXBUFFER, sizeof(char));
          strcpy(message, "ERROR ");
          strcat(message, commands[0]);
          strcat(message, " not supported over UDP\n");
          sendto(fd_udp, message, strlen(message), 0, (struct sockaddr*)& client, client_len);
        }
      }
    }
  }

  freeClients();                                                              // Free all of the clients in the clients array
  return EXIT_SUCCESS;
}
