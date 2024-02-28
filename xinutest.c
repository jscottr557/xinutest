#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <asm/termbits.h>
#include <poll.h>

#include "confirm_output.h"

#define TIMEOUT (250)
#define LIBSTDBUF_LOCATION "/usr/libexec/coreutils/libstdbuf.so"

const char *host_exe = "cs-console";
const char *host_exe_path = "/p/xinu/bin/";

const char *backend_prefix = "galileo";
char *backend;

const char *xinu_path = ".";
const char *xinu_name = "xinu.xbin";

//Writes a byte to the TTY described by term_fd
static void write_to_tty(int term_fd, char byte) {
  if(ioctl(term_fd, TIOCSTI, &byte) == -1)
  printf("\nerror writing to terminal\n");
}

//Write string followed by a carriage return to specified TTY
static void send_line(int term_fd, const char *string) {
  for(int i = 0; i < strlen(string); i++) {
    write_to_tty(term_fd, string[i]);
  }
  write_to_tty(term_fd, '\r');
  return;
}

//Put cs-console in command mode
static void command_mode(FILE *cs_console, int term_fd) {
  int status;
  do {
    write_to_tty(term_fd, 0x0);
    status = confirm_output(cs_console, "(command-mode)", "error", TIMEOUT);
  } while(status != STATE_SUCCEED);
  return;
}

//Driver "script" for testing cs-console
//Should be used in the parent process
//@param cs_console read end of the pipe cs-console writes to
//@param term_fd file descriptor for the TTY cs-console is attached to
int xinu_test(FILE *cs_console, int term_fd) { //Not happy about mixing FILE *s and raw file descriptor numbers but i cant be bothered to fix it rn
  if (cs_console == NULL || ferror(cs_console)) {
    printf("\neither you passed a null file pointer or the pipe is errored\n");
    return 1;
  }

  //confirm connection established
  int output_status = 0;
  do {
    output_status = confirm_output(cs_console, backend, "error", TIMEOUT);
    if(output_status == STATE_FAIL) {
      wait(NULL);
      printf("\nFailed to connect to %s\n", backend);
      return 1;
    }
  } while(output_status != STATE_SUCCEED);

  //put cs-console in command mode, this has its own error checking
  command_mode(cs_console, term_fd);

  //send input upload command
  do {
    do { //Get the prompt for the file name
      send_line(term_fd, "d");
      output_status = confirm_output(cs_console, "file:", "Illegal", TIMEOUT);
    } while(output_status != STATE_SUCCEED);

    send_line(term_fd, xinu_name); //send the file name
    output_status = confirm_output(cs_console, "cp-download complete", "No such file", TIMEOUT);
    if(output_status == STATE_FAIL) {
      do {
        send_line(term_fd, "q");
        output_status = confirm_output(cs_console, "SHUTDOWN", "error", TIMEOUT);
      } while(output_status != STATE_SUCCEED);
      wait(NULL);
      printf("\n>>>bad file name: %s<<<\n", xinu_name);
      return 1;
    }
  } while(output_status != STATE_SUCCEED);

  command_mode(cs_console, term_fd); //get back in command mode

  send_line(term_fd, "p"); //and powercycle the testbench

  char buffer = '\0';
  while(!feof(cs_console) && !ferror(cs_console)) { //infinite loop to relay output of cs-console to the terminal
    fread(&buffer, sizeof(char), 1, cs_console);
    fwrite(&buffer, sizeof(char), 1, stdout);
  }

  return 0;
}

int main(int argc, char **argv) {
  if(argc < 2)
  {
    printf("Usage: xinutest <backend number>\n");
    return 1;
  }
  
  //Get the host name
  char* backend_number = argv[1];
  char backend_buffer[strlen(backend_prefix) + strlen(backend_number) + 1];
  strcpy(backend_buffer, backend_prefix);
  strcat(backend_buffer, backend_number);
  backend = backend_buffer;

  //Figure out where cs-console lives
  char host_location[strlen(host_exe) + strlen(host_exe_path) + 1];
  strcpy(host_location, host_exe_path);
  strcat(host_location, host_exe);

  //cant use popen here because i need to turn off read/write buffering
  int mypipe[2];
  if(pipe(mypipe) == -1) {
    perror("pipe");
    return 1;
  }

  int term_fd = dup(STDIN_FILENO); //Save a copy of stdin

  pid_t child_pid = fork();
  if(child_pid == -1) {
    perror("fork");
    return 1;
  }

  int parent_status = 0;
  if(child_pid == 0) {
    if(dup2(mypipe[1], STDOUT_FILENO) == -1) {
      perror("cs-console dup2");
      return 1;
    }
    
    if(dup2(mypipe[1], STDERR_FILENO) == -1) { //this is almost certainly an awful idea, but i need err output from download command to check for bad files :)
      perror("cs-console dup2");
      return 1;
    }

    close(mypipe[1]); //resides where STDOUT was
    close(mypipe[0]); //child never uses this

    //Switch from full to line buffering for the child process (aka cs-console)
    //God bless Pádraig Brady, piggybacking off of his work for stdbuf
    //See: https://github.com/coreutils/coreutils/blob/master/src/stdbuf.c
    setenv("LD_PRELOAD", LIBSTDBUF_LOCATION, 1); //SPECIFIC TO XINU FRONTENDS' CONFIGURATION
    setenv("_STDBUF_O", "L", 1);

    execl(host_location, host_exe, backend, (char *) NULL);
  }
  else {
    if(dup2(mypipe[0], STDIN_FILENO) == -1) {
      perror("driver dup2");
      return 1;
    }
    close(mypipe[1]); //driver never uses this
    close(mypipe[0]); //resides where STDIN was
    
    setvbuf(stdout, NULL, _IONBF, 0); //turn off write buffering

    parent_status = xinu_test(stdin, term_fd);

    wait(NULL);
  }

  return parent_status;
}

