#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uv.h"

uv_cpu_info_t* self_cpu_getinfo(int* count);    // return the cores array; count returns the number of cores
void print_cpu_info(uv_cpu_info_t* info, int count);

// Somma dei cpu_times su tutti i core, presa in un istante.
// Sono contatori cumulativi dal boot: da soli non dicono il carico,
// serve la differenza tra due snapshot successivi (vedi cpu_load_pct).
typedef struct {
    uint64_t user, nice, sys, idle, irq;
} CpuTimesSnapshot;

CpuTimesSnapshot cpu_times_snapshot(void);

// Carico 0-100 nell'intervallo tra due snapshot: frazione di tempo
// non-idle sul totale. Se total_delta == 0 (snapshot troppo ravvicinati
// o primo campionamento) ritorna 0 invece di dividere per zero.
uint8_t cpu_load_pct(CpuTimesSnapshot prev, CpuTimesSnapshot curr);

// Capacita' statica: numero di core e velocita' media in MHz.
void cpu_capacity(uint16_t* n_cores, uint16_t* avg_mhz);

#endif
