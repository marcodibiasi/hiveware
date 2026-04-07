#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void exit_error(char* msg){
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}


