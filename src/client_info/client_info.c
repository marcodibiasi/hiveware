#include "client_info.h"

uv_cpu_info_t* client_cpu_get_info(void){

    uv_cpu_info_t* client_cpu;
    int count = 1;
    uv_cpu_info(&client_cpu,&count);

    return client_cpu;

}

void free_client_cpu_get_info(uv_cpu_info_t* client_cpu, int count){
    uv_free_cpu_info(client_cpu,count);
}
