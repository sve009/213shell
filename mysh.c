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

// separate("ls -l -w -something");
// -> curr = "ls ...
// -> input = "ls ...
// -> i = 0
// After first strsep:
// -> curr = "ls\0"
// -> input = "-l ..."
// -> i = 1

struct command {
  char delimiter;
  char* comm;
};


void print_list(char** list) {
  int i = 0;
  while (list[i] != NULL) {
    printf("%s\n", list[i]);
    i++;
  }
}

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

    char* delim_position;
    struct command* commands[MAX_ARGS];
    int i = 0;

    while (true) {

      delim_position = strpbrk(line, ";&");

      if (delim_position == NULL) {
        commands[i++] = malloc(sizeof(struct command));
        commands[i-1]->delimiter = '\0';
        commands[i-1]->comm = line;

        if (strcmp(line, "\n") == 0) commands[--i] = NULL;
        break;
      }

      char delim = *delim_position;
      *delim_position = '\0';

      commands[i++] = malloc(sizeof(struct command));
      commands[i-1]->delimiter = delim;
      commands[i-1]->comm = line;
      line = delim_position + sizeof(char);
    }

    commands[i] = NULL;

    // print_list(commands);

    for (int j = 0; j < i; j++) {
      if (commands[j]->comm == NULL) continue;

      char ** words = separate(commands[j]->comm);

      // if the inputted command is cd then call chdir function
      if (strcmp (words[0], "cd") == 0) {
        if (words[1] !=NULL) {
          chdir(words[1]);
        }
        continue;
      }

      //exit the program if exit command is typed
      else if (strcmp (words[0], "exit") == 0) {
        break;
      }

      // TODO: Execute the command instead of printing it below
      pid_t (child_id) = fork();


      if (child_id == -1){
        fprintf(stderr, "Fork failed\n");
        continue;
      }
      else if (child_id == 0){
        execvp (words[0], words);
        printf("execvp failed\n");
      }
      else {
        int status = -1;
        if (commands[j]->delimiter != '&' && wait(&status) == -1) {
          fprintf(stderr, "Waiting on child failed\n");
          continue;
        }

        // Maybe fix this later?
        if (status != -1){
          printf("[%s exited with status %d]\n", words[0], WEXITSTATUS(status));
        }


      }
    }
    int ret;
    int status;
    while ((ret = waitpid(-1, &status, WNOHANG)) > 0) {
      printf("[background process %d exited with status %d]\n", ret, WEXITSTATUS(status));
    }
  }

  // If we read in at least one line, free this space
  if (line != NULL) {
    free(line);
  }

  return 0;
}
