#include <stdio.h>
#include <stdlib.h>
#include "user_console.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        if (argc < 2)
            printf("Arguments missing!\nUsage: user_console *console ID*\n");
        else
            printf("Too many arguments!\n");
        exit(1);
    }
}