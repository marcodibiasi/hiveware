#include <stdio.h>
#include <stdlib.h>
#include "uv.h"

uv_cpu_info_t* client_cpu_get_info(void); /*Chiama questa funzione*/
void free_client_cpu_get_info(uv_cpu_info_t* client_cpu, int count);
