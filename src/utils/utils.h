#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

void exit_error(char* msg);
bool mac_compare(uint8_t* mac_address_1, uint8_t* mac_address_2);

#endif // UTILS_H

