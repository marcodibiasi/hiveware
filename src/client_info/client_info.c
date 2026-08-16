#include <stdio.h>
#include "uv.h"
#include "client_info.h"


CpuTimesSnapshot cpu_times_snapshot(void){
    CpuTimesSnapshot snap = {0};

    int count;
    uv_cpu_info_t* info;
    if(uv_cpu_info(&info, &count) != 0)
        return snap; // ritorna tutto a zero, il chiamante gestisce il caso

    for(int i = 0; i < count; i++){
        snap.user += info[i].cpu_times.user;
        snap.nice += info[i].cpu_times.nice;
        snap.sys  += info[i].cpu_times.sys;
        snap.idle += info[i].cpu_times.idle;
        snap.irq  += info[i].cpu_times.irq;
    }

    uv_free_cpu_info(info, count);
    return snap;
}


uint8_t cpu_load_pct(CpuTimesSnapshot prev, CpuTimesSnapshot curr){
    uint64_t prev_total = prev.user + prev.nice + prev.sys + prev.idle + prev.irq;
    uint64_t curr_total = curr.user + curr.nice + curr.sys + curr.idle + curr.irq;

    if(curr_total <= prev_total)
        return 0; // primo campionamento o intervallo troppo corto

    uint64_t total_delta = curr_total - prev_total;
    uint64_t idle_delta = curr.idle - prev.idle;

    if(idle_delta > total_delta)
        return 0; // dato incoerente (es. overflow contatori), fallback sicuro

    return (uint8_t)(100 * (total_delta - idle_delta) / total_delta);
}


void cpu_capacity(uint16_t* n_cores, uint16_t* avg_mhz){
    int count;
    uv_cpu_info_t* info;

    *n_cores = 0;
    *avg_mhz = 0;

    if(uv_cpu_info(&info, &count) != 0 || count <= 0)
        return;

    uint64_t total_mhz = 0;
    for(int i = 0; i < count; i++)
        total_mhz += (uint64_t)info[i].speed;

    *n_cores = (uint16_t)count;
    *avg_mhz = (uint16_t)(total_mhz / count);

    uv_free_cpu_info(info, count);
}


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
