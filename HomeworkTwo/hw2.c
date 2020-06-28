#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdbool.h>

int spaces = 0;
bool isBack = false;

/*********************************************************************************
commandFind():
Takes a argument, and the current path. It finds the command by looping through
the path
*********************************************************************************/
char* commandFind(char* argument, char* myPath)
{
  char* pathPtr = calloc(strlen(myPath) + 1, sizeof(char));       // Copying over the path for maipulation
  strcpy(pathPtr, myPath);

  struct stat status;                                             // used to return the status from the lstat call
  int found = -1;

  char * s = ":";
  char* temp = strtok(pathPtr, s);                                // Dont want the ":", drops it into temp
  while(temp!=NULL)                                               // splits each segment by the ":", checking if
  {                                                               // the command is in that path !
    char* possiblePath = calloc(strlen(argument) + strlen(pathPtr) + 2, sizeof(char));
    strcpy(possiblePath, temp);
    strcat(possiblePath, "/");
    strcat(possiblePath, argument);                               // adds the argument to each location in the path

     printf("path: %s\n",possiblePath );
    found = lstat(possiblePath, &status);                         // lstat checks to see if the command exists
    if(found == 0)                                                 // if the command is found ...
    {
      free(pathPtr);
      return(possiblePath);
    }
    free(possiblePath);
    temp = strtok(NULL, s);
  }
  free(pathPtr);
  fprintf(stderr, "ERROR: command \"%s\" not found\n", argument);
  return " ";
}

/*********************************************************************************
read_line():
This function is called by main within the while loop. It is expected to read
from stdin and output the current directory path, and the current line in the
file / command prompt.
*********************************************************************************/
// void read_line(char[] command)
// {
//   fgets(*command, 1024, stdin);
// }

/*********************************************************************************
parse_line():
This function is called by macommandFindin within the while loop. It is expected to take
a string command, which is a command from the terminal. It breaks the command
into arguments as necessry, and stores them in a 2D array.
*********************************************************************************/
char** parse_line(char* command, int* all)
{
  int i = 0;
  while(command[i] != '\0')
  {
    if(command[i] == ' ' || command[i] == '\n') spaces+=1;
    i+=1;
  }
  char **commands = calloc(spaces +1, sizeof(char*));
  char *argument;
  char *s = " \n\0";                          // Delimeter for what strtok is going to split on
  int cIndex = 0;                             // The current index of the command array
  // Splits the line by spaces... and continues until there are no spaces left
  // This is done utilizing the strtok function in the c library

  argument = strtok(command, s);              // Retrieves the first argment
  while(argument != NULL)                     // Loops through the next arguments
  {
    if(strcmp(argument, "&") == 0)
    {
      isBack = true;
      break;
    }
    //commands[cIndex] = argument;              // assign the argument to the command array
    commands[cIndex] = argument;
    cIndex+=1;
    argument = strtok(NULL, s);
  }

  commands[cIndex] = NULL;
  return commands;
}

/*********************************************************************************
execute_cd():
Executes the cd built-in, which is different from the normal fork calls for other
argument types.
*********************************************************************************/
void execute_cd(char** arguments)
{
  if(arguments[1] == NULL)
  {
    chdir(getenv("HOME"));
  }
  else
  {
    if (chdir(arguments[1]) != 0)
    {
      fprintf(stderr, "chdir() failed: Not a directory\n");
      return;
    }
  }
  return;
}

/*********************************************************************************
execute_fork():
When there is an agrument that is not a pipe or a cd built-in, this function is
called. It creates a child process ... and runs the exec. Called by execute().
*********************************************************************************/
void execute_fork(char** arguments, char* myPath)
{
  char* commandPath = commandFind(arguments[0], myPath);  // Gets the path to the exec.
  if(strcmp(commandPath, " ") == 0) return;

  pid_t pid;
  pid = fork();                                         // Create a child process

  if (pid < 0) perror("ERROR: fork() failed\n");        // If the fork fails
  else if (pid == 0)                                    // Enter the child process
    {
      execv(commandPath, arguments);                    // Terminates the child process ...
      exit(EXIT_SUCCESS);
    }
    else                                                // Enter the parent process
    {
      if(isBack == true)printf("[running background process \"%s\"]\n", arguments[0]);
      else waitpid(pid, 0, 0);                                                          // Non-Background Process
    }
  free(commandPath);
  return;
}

/*********************************************************************************
execute_pipe():
When there is a pipe, this function is called. Divides the arguments array in two
(pipe1 = left of the pipe, pipe2 = right of the pipe)
*********************************************************************************/
void execute_pipe(char** arguments, char* myPath)
{
  // First, I need to split arguments on the "|" command for each array
  int pipe_location = 0;
  int commands_1 = 0;                                     // The number of commands in the pipe1 array
  int commands_2 = 1;                                     // The number of commands in the pipe2 array
  for(int i =0; arguments[i] != NULL; i++)
  {
    if(strcmp(arguments[i],"|") == 0) pipe_location = i;  // gets the location of the pipe
    if(pipe_location == 0) commands_1 +=1;                // adds to number of indecies in pipe1
    else commands_2 += 1;                                 // adds to number of indecies in pipe2
  }
  char ** pipe1 = calloc(commands_1+1, sizeof(char*));    // allocating the necessary memory
  char ** pipe2 = calloc(commands_2+1, sizeof(char*));

  int j = 0;
  while(strcmp(arguments[j], "|") != 0)                   // Assigning the first values to pipe1 array
  {
    pipe1[j] = arguments[j];
    j+=1;
  }
  j+=1;                                                   // Currently on "|", move to the next
  int locTwo = 0;

  while(arguments[j] != NULL)
  {
    pipe2[locTwo] = arguments[j];
    j+=1;
    locTwo +=1;
  }

  // Now, moving on to the forking + piping
  char* commandPath1 = commandFind(pipe1[0], myPath);     // Gets the path to the exec for pipe1 value
  char* commandPath2 = commandFind(pipe2[0], myPath);     // Gets the path to the exec for pipe2 value
  if(strcmp(commandPath1, " ") == 0)
  {
    free(pipe1);
    free(pipe2);
  return;
}
  if(strcmp(commandPath2, " ") == 0)
  {
    free(commandPath1);
    free(pipe1);
    free(pipe2);
    return;
}

  int pipefd[2];                                          // Opens the pipe
  int rc = pipe(pipefd);
  if(rc ==-1) perror("ERROR: pipe() failed\n");         // Failed pipe
  else
  {
    pid_t pid;
    pid = fork();
    if (pid < 0) perror("ERROR: fork() failed\n");      // Failed fork
    else if (pid == 0)                                    // Child process for first fork
    {
      close(pipefd[0]);
      dup2(pipefd[1], 1);                                 // Now, std::out becomes writing to the buffer
      execv(commandPath1, pipe1);
      close(pipefd[1]);                                   // Can close the write location, as it is in fd[1] (write)
    }
    else                                                  // Parent Process for first fork
    {
      pid_t pid2;
      pid2 = fork();
      if(pid2 == 0)
      {
        close(pipefd[1]);
        dup2(pipefd[0], 0);                               // Now, the fd read is now std::in
        execv(commandPath2, pipe2);
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
      }
      else
      {
        close(pipefd[0]);
        close(pipefd[1]);
        if(isBack == false)
        {
          waitpid(pid, 0, 0);                             // Wait for the first process
          waitpid(pid2, 0, 0);                            // Wait for the second process
        }
        else
        {
          printf("[running background process \"%s\"]\n",pipe1[0]);  // Running Process one in background
          printf("[running background process \"%s\"]\n",pipe2[0]);       // Running Process two in background
          free(pipe1);
          free(pipe2);
        }
      }
    }
  }
  free(commandPath1);
  free(commandPath2);
  free(pipe1);
  free(pipe2);
  return;
}

/*********************************************************************************
execute():
Is called by main, when there is an argument to execute. Separates the execution
type into a pipe, normal exec, or built-in function, and calls a method for each.
*********************************************************************************/
void execute(char** arguments, char* myPath)
{
  bool isPipe = false;                              // Boolean if there is a pipe
  if(arguments[0] == NULL) return;                  // Checks if there is anyting !

  char** o = arguments;
  while(*o != NULL)
  {
    if(strcmp(*o, "|") == 0) isPipe = true;                               // Determines if there is a pipe command
    o++;
  }

  if(strcmp(arguments[0], "cd\0")==0)                                     // Calls for cd statements
    return execute_cd(arguments);
  if(isPipe == false) execute_fork(arguments, myPath);                    // Calls for non-pipe execs
  else if(isPipe == true) return execute_pipe(arguments, myPath);         // Calls for pipe execs



  return;
}

/*********************************************************************************
main():
Acts as the infinite loop, calling each process as necessary for the read_line,
*********************************************************************************/
int main()
{
  setvbuf( stdout, NULL, _IONBF, 0 );
  char* myPath = calloc(64,sizeof(char));       // The current path

  // Setting the path, starts a bin if a path cannot be found
  if(getenv("MYPATH") == NULL) strcpy(myPath, "/bin:.");                   // starts at the bin
  else strcpy(myPath, getenv("MYPATH"));

  while(1)                                      // The shell loops over input until ... exit is called
  {
    // First, I kill all background processes
    int status = 0;
    pid_t pidKill = waitpid(-1, &status, WNOHANG);
    if(pidKill > 0)
    {
      if(WIFEXITED(status)) printf("[process %d terminated with exit status %d]\n", pidKill, WEXITSTATUS(status));
      else printf("[process %d terminated abnormally]\n",pidKill);
    }

    char dir[1024];
    getcwd(dir, sizeof(dir));
    printf("%s$ ", dir);

    char command[1024];
    fgets(command, 1024, stdin);
    if(strcmp(command, "exit") == 0) break;                 // The shell terminates on input 'exit'
    if(strcmp(command, "") == 0) continue;                  // Moves on to the next if nothing is inputted

    char **arguments;                                       // A 2D array of arguments for each command
    int all = 0;
    arguments = parse_line(command, &all);                  // Splits the command into arguments ... and executes as necessary

    execute(arguments, myPath);                             // Executes the argument values stored
    if(isBack == true) isBack = false;
    // Freeing Memory

    free(arguments);
  }

  printf("bye\n");
  free(myPath);
  return EXIT_SUCCESS;
}
