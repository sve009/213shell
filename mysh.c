#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// This is the maximum number of arguments your shell should handle for one command
#define MAX_ARGS 128

// Tracks both the limiter and the actual command to run
// Each line is broken up into a list of commands
struct command {
  char delimiter;
  char* comm;
};

// Break the given string into the program and arguments
//   Note: Calls malloc, user is expected to later free
//         the list of arguments that is returned.
char** separate(char* input) {
  char** list = malloc(sizeof(char*) * MAX_ARGS);

  // Use strsep to break arguments
  int i = 0;

  // Declare word var
  char *found;

  // Grab each argument and put it in list
  while ( (found = strsep(&input, " \n\0")) != NULL) {
    // Only add if nonempty
    if (strlen(found) != 0) list[i++] = found;
  }

  // Add null to end
  list[i] = NULL;

  // Return our allocated list
  return list;
}

int main(int argc, char** argv) {
  // If there was a command line option passed in, use that file instead of stdin
  if (argc == 2) {
    // Try to open the file
    int new_input = open(argv[1], O_RDONLY);
    if (new_input == -1) {
      fprintf(stderr, "Failed to open input file %s\n", argv[1]);
      exit(1);
    }

    // Now swap this file in and use it as stdin
    if (dup2(new_input, STDIN_FILENO) == -1) {
      fprintf(stderr, "Failed to set new file as input\n");
      exit(2);
    }
  }

  char* line = NULL;     // Pointer that will hold the line we read in
  char* orig_line = NULL; // Copy of line
  size_t line_size = 0;  // The number of bytes available in line

  // Loop forever
  while (true) {
    // Print the shell prompt
    printf("$ ");

    // Get a line of stdin, storing the string pointer in line
    if (getline(&line, &line_size, stdin) == -1) {
      if (errno == EINVAL) {
        perror("Unable to read command line");
        exit(2);
      } else {
        // Must have been end of file (ctrl+D)
        printf("\nShutting do<sys/wait.h>wn...\n");

        // Exit the infinite loop
        break;
      }
    }

    orig_line = line;

    // holds the current delim position
    char* delim_position;

    // holds the list of commands
    struct command* commands[MAX_ARGS];

    // Make sure first element of commands is NULL so if
    //   it's empty no issues come up while trying to free memory.
    commands[0] = NULL;

    // command counter
    int i = 0;

    // This loop breaks up the line into individual
    //   commands complete with the command and which
    //   delimiter was used.
    while (true) {

      // Breaks up line at delimiter
      delim_position = strpbrk(line, ";&");

      // If no delimiters remain
      if (delim_position == NULL) {
        // Create the final command
        commands[i++] = malloc(sizeof(struct command));
        // Make sure delimiter is ;
        commands[i-1]->delimiter = '\0';
        commands[i-1]->comm = line;

        // if all that remains is new line then delete the last command
        if (strcmp(line, "\n") == 0) commands[--i] = NULL;

        //stops the loop
        break;
      }

      //save the delimiter
      char delim = *delim_position;
      //makes the current delimeter null
      *delim_position = '\0';

      //allocating space to hold commands
      commands[i++] = malloc(sizeof(struct command));
      commands[i-1]->delimiter = delim;
      commands[i-1]->comm = line;

      //advance the line to next command
      line = delim_position + sizeof(char);
    }

    //null terminate the list of commands
    commands[i] = NULL;

    //iterate through commands
    for (int j = 0; j < i; j++) {

      //don't run null commands
      if (commands[j]->comm == NULL) continue;

      //break command into program and arguments
      char ** words = separate(commands[j]->comm);

      // if the inputted command is cd then call chdir function
      if (strcmp (words[0], "cd") == 0) {
        if (words[1] !=NULL) {
          // Change the directory, if it fails log the error
          if (chdir(words[1]) == -1) {
            perror("Changing directory failed");
          }
        }
        continue;
      }

      //exit the program if exit command is typed
      else if (strcmp (words[0], "exit") == 0) {
        exit(0);
      }

      // Fork to create new process
      pid_t child_id = fork();

      //if fork fails
      if (child_id == -1){
        perror("Fork failed");
        continue;
      }

      // if in child run the command
      else if (child_id == 0){
        // Run the program, if it fails log error
        if (execvp (words[0], words) == -1) {
          perror("execvp failed");
          exit(errno);
        }
      }
      else {
        //declare status
        int status = -1;

        //if not a background command and waitpid did fail
        if (commands[j]->delimiter != '&' && waitpid(child_id, &status,0) == -1) {
          fprintf(stderr, "Waiting on child failed\n");
          continue;
        } //if not background command
        else if (commands[j]->delimiter != '&') {
          // This implies that we blocked and waiting didn't fail
          //   so log the command completion.
          printf("[%s exited with status %d]\n", words[0], WEXITSTATUS(status));
        }
      }

      // No memory leaks for us (just for you Professor Curtsinger :) )
      if (words != NULL) {
        free(words);
      }
    }

    //return value of wait pid
    int ret;
    //status value of wait pid
    int status;

    //while any background processes have finished log them all
    while ((ret = waitpid(-1, &status, WNOHANG)) > 0) {
      printf("[background process %d exited with status %d]\n", ret, WEXITSTATUS(status));
    }

    // Free all of the commands that we allocated
    int k = 0;
    while (commands[k] != NULL) {
      if (commands[k] != NULL) free(commands[k]);
      commands[k++] = NULL;
    }
  }

  // If we read in at least one line, free this space
  if (line != NULL) {
    free(orig_line);
  }

  return 0;
}
