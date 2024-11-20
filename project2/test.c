#include <stdio.h>

int main() {
    int a = 5;
    int b = 6;
    int *a_p = &a;
    int *b_p = &b;
    b_p = a_p;
    printf("a_p: %p, b_p: %p", a_p, b_p);
    if (b_p == a_p) {
        printf("yeah\n");
    }

}