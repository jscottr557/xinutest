#include "confirm_output.h"

static void clear_string(char *str) {
  for(int i = 0; i < strlen(str); i++) {
    str[i] = '\0';
  }
  return;
}

static char *wordsearch(char next_letter, char *buffer, char *needle) { //Same algorithim you use for the puzzle
    if(buffer == NULL) {
      return (char *) calloc(1, strlen(needle) + 1); //initilization, buffer should be full of NULL by ascii definition
    }

    int index = strlen(buffer); //this protects against buffer overflows, only way to "keep going" is null terminator which does not extend string length therefore does not move write pointer
    if(next_letter == needle[index]) { //The next letter is contained in the correct needle index
      buffer[index] = next_letter;
    }
    else { //The next letter is not contained in the correct needle index
      clear_string(buffer);
    }

    return buffer;
}

int confirm_output(FILE *fd, char *succeed, char *fail, int timeout_ms) { 
  if(fd == NULL) return NULL_PTR_ARG;
  if(ferror(fd)) return STATE_FILE_ERR;
  if(succeed == NULL) return NULL_PTR_ARG;
  if(fail == NULL) return NULL_PTR_ARG;

  
  if(fcntl(fileno(fd), F_SETFL, fcntl(fileno(fd), F_GETFL) | O_NONBLOCK) != 0) { //turn off blocking reads from pipe
      return STATE_FILE_ERR; 
  } 

  char buffer = '\0';
  static char *succeed_buffer = NULL; //static allows preservation of state in between write bursts
  static char *fail_buffer = NULL; //static allows preservation of state in between write bursts
  if(succeed_buffer == NULL) succeed_buffer = wordsearch('\0', NULL, succeed);
  if(fail_buffer == NULL) fail_buffer = wordsearch('\0', NULL, fail);

  struct pollfd term_poll;
  struct pollfd *term_poll_ptr = &term_poll;
  term_poll_ptr->fd = fileno(fd);
  term_poll_ptr->events = POLLIN; 

  int state = STATE_MATCHING;
  while(poll(term_poll_ptr, 1, timeout_ms) && state == STATE_MATCHING) { // look for something to read
    if(read(fileno(fd), &buffer, sizeof(char)) == -1) printf ("file read error");
    write(STDOUT_FILENO, &buffer, sizeof(char)); //Update the screen //TODO: GET RID OF THIS SIDE EFFECT

    if(strcmp(wordsearch(buffer, succeed_buffer, succeed), succeed) == 0) { //we find the succeed string
      state = STATE_SUCCEED;
    }
    else if(strcmp(wordsearch(buffer, fail_buffer, fail), fail) == 0) { //we find the fail string
      state = STATE_FAIL;
    }
  }

  if(fcntl(fileno(fd), F_SETFL, fcntl(fileno(fd), F_GETFL) & ~O_NONBLOCK) != 0) { //turn on blocking reads from pipe
      return STATE_FILE_ERR; 
  } 

  if(state == STATE_SUCCEED || state == STATE_FAIL) { //only clear the buffers if we find a match
    free(succeed_buffer);
    free(fail_buffer);
    succeed_buffer = NULL;
    fail_buffer = NULL;
  }
  else {
    state = STATE_NO_MATCH;
  }
  return state;
}
