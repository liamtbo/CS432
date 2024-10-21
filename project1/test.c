#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for stdin_fileno


int main() {
    char *s = "fuck hamas";
    printf("%s", s); fflush(stdout);
    char del = '\b';
    // printf("%s", s);
    sleep(2);
    printf("%c", del);

}