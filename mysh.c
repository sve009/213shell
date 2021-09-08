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

char** separate(char* input) {
  char** list = malloc(sizeof(char*) * MAX_ARGS);

  // Use strsep to break arguments
  char* curr = input;
  int i = 0;
  strsep(&input, " \n");
  list[i++] = curr;
  while (curr != input) {
    // Update current to be farmost pointer
    curr = input;

    // If string is empty skip it
    if (strlen(curr) == 0) continue;

    // Add current to list
    list[i++] = curr;

    // Move input to next argument
    strsep(&input, " \n");
  }

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

    // TODO: Execute the command instead of printing it below
    pid_t (child_id) = fork();
    char ** words = separate(line);

    if (child_id == -1){
      fprintf(stderr, "Fork failed\n");
      continue;
    }
    else if (child_id == 0){
      printf("This is the child\n");
      execvp (words[0], words);
    }
    else {
      int status;
      if (wait(&status) == -1) {
        fprintf(stderr, "Waiting on child failed\n");
        continue;
      }
      // Maybe fix this later?
      printf("[%s exited with status %d]\n", words[0], WEXITSTATUS(status));
    }

    printf("Received command: %s\n", line);
  }

  // If we read in at least one line, free this space
  if (line != NULL) {
    free(line);
  }

  return 0;
}
