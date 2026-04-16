#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include "uv.h"

uv_cpu_info_t* self_cpu_getinfo(int* count);    // return the cores array; count returns the number of cores
void print_cpu_info(uv_cpu_info_t* info, int count);


#endif
