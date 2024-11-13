#include <termios.h>
#include <unistd.h>
#include "raw.h"

/* See raw.h for usage information */
/*
static
    lifetime for the entire program and holds value aftre function ends
    only accessable within file (or function) it's created in
struct termios
    termios stores config for terminal i/o interface in unix os
oldterm
    store original terminal config settings
*/
static struct termios oldterm;

/* Returns -1 on error, 0 on success */
int raw_mode (void)
{
        struct termios term;

        if (tcgetattr(STDIN_FILENO, &term) != 0) return -1;
    
        oldterm = term; // save original terminal attributes
        /* Turn off echoing (displaying) of typed charaters in terminal */ 
        term.c_lflag &= ~(ECHO);
        // canonical mode (cooked mode) buffers input in terminal untill \n is received
        term.c_lflag &= ~(ICANON);  /* Turn off line-based input */
        // min num of chars that must be available for read op to return/stop blocking
        term.c_cc[VMIN] = 1; // min number of chars to read
        // 0 means read op will block indefinitely until input is availbale
        term.c_cc[VTIME] = 0; // timeout for reading chars (0 means no timeout)
        tcsetattr(STDIN_FILENO, TCSADRAIN, &term); // put the new terminal attrinbutes into action

        return 0;
}

void cooked_mode (void)    
{  
        // reset terminal i/o to default
        tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
}
