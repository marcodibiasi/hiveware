#include <stdlib.h>
#include <string.h>
#include "job_registry.h"
#include "utils.h"


JobRegistry* job_registry_init(void){
    JobRegistry* reg = calloc(1, sizeof(JobRegistry));
    if(!reg) exit_error("Calloc Failed (job_registry)");

    if(pthread_mutex_init(&reg->registry_mutex, NULL) != 0)
        exit_error("pthread_mutex_init registry_mutex");

    return reg;
}


void job_registry_destroy(JobRegistry* reg){
    if(!reg) return;
    pthread_mutex_destroy(&reg->registry_mutex);
    free(reg); // i Job puntati non vengono toccati, vedi header
}


int job_registry_add(JobRegistry* reg, Job* job){
    if(!reg || !job) return -1;

    pthread_mutex_lock(&reg->registry_mutex);
    if(reg->n_jobs >= MAX_ACTIVE_JOBS){
        pthread_mutex_unlock(&reg->registry_mutex);
        return -1;
    }
    reg->jobs[reg->n_jobs++] = job;
    pthread_mutex_unlock(&reg->registry_mutex);

    return 0;
}


int job_registry_remove(JobRegistry* reg, uuid_t job_id){
    if(!reg) return -1;

    pthread_mutex_lock(&reg->registry_mutex);
    for(int i = 0; i < reg->n_jobs; i++){
        if(uuid_compare(reg->jobs[i]->job_id, job_id) == 0){
            reg->jobs[i] = reg->jobs[reg->n_jobs - 1]; // packed array, come peer_disc
            reg->n_jobs--;
            pthread_mutex_unlock(&reg->registry_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&reg->registry_mutex);
    return -1;
}


Job* job_registry_find(JobRegistry* reg, uuid_t job_id){
    if(!reg) return NULL;

    pthread_mutex_lock(&reg->registry_mutex);
    Job* found = NULL;
    for(int i = 0; i < reg->n_jobs; i++){
        if(uuid_compare(reg->jobs[i]->job_id, job_id) == 0){
            found = reg->jobs[i];
            break;
        }
    }
    pthread_mutex_unlock(&reg->registry_mutex);

    return found;
}
