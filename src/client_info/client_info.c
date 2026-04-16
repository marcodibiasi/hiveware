#include <stdio.h>
#include "uv.h"
#include "client_info.h"


uv_cpu_info_t* self_cpu_getinfo(int* count){
    uv_cpu_info_t* client_cpu;
    uv_cpu_info(&client_cpu, count);

    return client_cpu;
}


void print_cpu_info(uv_cpu_info_t* info, int count){
    printf("=== CPU INFO ===\n");
    printf("cores: %d\n\n", count);

    for(int i = 0; i < count; i++){
        printf("[core %d]\n", i);
        printf("  model: %s\n", info[i].model);
        printf("  speed: %d MHz\n", info[i].speed);

        printf("  times:\n");
        printf("    user:  %llu\n", (unsigned long long)info[i].cpu_times.user);
        printf("    nice:  %llu\n", (unsigned long long)info[i].cpu_times.nice);
        printf("    sys:   %llu\n", (unsigned long long)info[i].cpu_times.sys);
        printf("    idle:  %llu\n", (unsigned long long)info[i].cpu_times.idle);
        printf("    irq:   %llu\n", (unsigned long long)info[i].cpu_times.irq);

        printf("\n");
    }
}
