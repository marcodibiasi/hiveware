#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <uuid/uuid.h>
#include <time.h>

void exit_error(char* msg);
void print_uuid(uuid_t id);
void addr_to_string(const struct sockaddr_in *addr, char out[32]);
double timespec_to_double(const struct timespec *t);

#endif // UTILS_H

