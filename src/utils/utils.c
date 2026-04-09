#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include "utils.h"

void exit_error(char* msg){
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

void print_uuid(uuid_t id){
    char id_str[37];
    uuid_unparse(id, id_str);
    printf("%s\n", id_str);
}

