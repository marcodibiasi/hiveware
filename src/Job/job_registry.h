#ifndef JOB_REGISTRY_H
#define JOB_REGISTRY_H

#include <pthread.h>
#include <uuid/uuid.h>
#include "job.h"

#define MAX_ACTIVE_JOBS 64

/*
 * Tiene traccia dei Job di cui questo nodo e' coordinator, indicizzati
 * per job_id. Serve a tcp_conn_handler per instradare i TASK_RESULT /
 * TASK_ERROR in arrivo dalla rete verso il Job giusto.
 *
 * Non possiede i Job: li traccia solo, la memoria resta responsabilita'
 * di chi li ha creati con job_create() / distrutti con job_destroy().
 */
typedef struct {
    Job* jobs[MAX_ACTIVE_JOBS];
    int n_jobs;
    pthread_mutex_t registry_mutex;
} JobRegistry;

JobRegistry* job_registry_init(void);
void job_registry_destroy(JobRegistry* reg); // libera solo il registro, non i Job

int job_registry_add(JobRegistry* reg, Job* job);
int job_registry_remove(JobRegistry* reg, uuid_t job_id);
Job* job_registry_find(JobRegistry* reg, uuid_t job_id);

#endif
