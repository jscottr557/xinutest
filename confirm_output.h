#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <asm/termbits.h>
#include <poll.h>
#include <errno.h>

#define STATE_MATCHING (0)
#define STATE_SUCCEED (1)
#define STATE_FAIL (2)
#define STATE_NO_MATCH (3)
#define STATE_FILE_ERR (4)
#define NULL_PTR_ARG (5)


/* continuously searchs a file's stream for one of two strings */
/* note: allocates internal buffers based on succeed/fail, always make sure that you get either STATE_SUCCEED or STATE_FAIL before moving on or you will leak*/
/* returns STATE_SUCCEED if succeed string is found */
/* returns STATE_FAIL if fail string is found */
/* returns STATE_NO_MATCH if timeout_ms ms pass wthout new data to read (-1 for no timeout, 0 for no block)*/
/* returns STATE_FILE_ERR if there is a read error */
/* returns NULL_PTR_ARG if any of the pointer arguments are NULL */
int confirm_output(FILE *fd, char *succeed, char *fail, int timeout_ms);