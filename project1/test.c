#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for stdin_fileno
#include <sys/select.h>
#include <string.h>

int main() {
    char s[3][32] = {
        "ball",
        "sack",
        "asfasfad",
    };
    strcpy(s[0], " ");
    printf("%s\n", s[0]);
}