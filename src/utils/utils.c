#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include <arpa/inet.h>
#include <string.h>
#include "utils.h"

void exit_error(char* msg){
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void print_uuid(uuid_t id){
    char id_str[37];
    uuid_unparse(id, id_str);
    printf("%s\n", id_str);
}


void addr_to_string(const struct sockaddr_in *addr, char out[32]) {
    inet_ntop(AF_INET, &addr->sin_addr, out, 32);
    size_t len = strlen(out);
    snprintf(out + len, 32 - len, ":%d", ntohs(addr->sin_port));
}


double timespec_to_double(const struct timespec *t) {
    return (double)t->tv_sec + (double)t->tv_nsec / 1e9;
}
