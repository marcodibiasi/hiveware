#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include "job.h"
#include "../utils/utils.h"


Job* job_create(uint32_t n_tasks){
    Job* job = calloc(1, sizeof(Job));
    if(!job) exit_error("Calloc Failed (job)");

    job->tasks = calloc(n_tasks, sizeof(Task));
    if(!job->tasks) exit_error("Calloc Failed (job->tasks)");

    for(uint32_t i = 0; i < n_tasks; i++)
        job->tasks[i].task_id = i; // status resta TASK_PENDING (0) finche' non popolato

    job->n_tasks = n_tasks;
    job->n_done = 0;
    uuid_generate_random(job->job_id);

    if(pthread_mutex_init(&job->job_mutex, NULL) != 0)
        exit_error("pthread_mutex_init job_mutex");

    return job;
}


void job_destroy(Job* job){
    if(!job) return;

    for(uint32_t i = 0; i < job->n_tasks; i++){
        free(job->tasks[i].deps);
        free(job->tasks[i].payload);
        free(job->tasks[i].result);
    }
    free(job->tasks);

    pthread_mutex_destroy(&job->job_mutex);
    free(job);
}


int job_set_task(Job* job, uint32_t task_id, uint32_t* deps, uint32_t n_deps,
                  void* payload, size_t payload_len){
    if(!job || task_id >= job->n_tasks)
        return -1;

    pthread_mutex_lock(&job->job_mutex);
    Task* t = &job->tasks[task_id];

    if(n_deps > 0){
        t->deps = malloc(n_deps * sizeof(uint32_t));
        if(!t->deps){
            pthread_mutex_unlock(&job->job_mutex);
            exit_error("malloc failed (task deps)");
        }
        memcpy(t->deps, deps, n_deps * sizeof(uint32_t));
    }
    t->n_deps = n_deps;
    t->n_deps_done = 0;

    t->payload = payload;
    t->payload_len = payload_len;

    t->status = (n_deps == 0) ? TASK_READY : TASK_PENDING;

    pthread_mutex_unlock(&job->job_mutex);
    return 0;
}


uint32_t* job_get_ready_tasks(Job* job, uint32_t* out_count){
    if(!job || !out_count) return NULL;

    pthread_mutex_lock(&job->job_mutex);

    uint32_t count = 0;
    for(uint32_t i = 0; i < job->n_tasks; i++)
        if(job->tasks[i].status == TASK_READY)
            count++;

    uint32_t* ready = NULL;
    if(count > 0){
        ready = malloc(count * sizeof(uint32_t));
        if(!ready){
            pthread_mutex_unlock(&job->job_mutex);
            exit_error("malloc failed (ready list)");
        }
        uint32_t j = 0;
        for(uint32_t i = 0; i < job->n_tasks; i++)
            if(job->tasks[i].status == TASK_READY)
                ready[j++] = job->tasks[i].task_id;
    }

    pthread_mutex_unlock(&job->job_mutex);
    *out_count = count;
    return ready;
}


int job_mark_assigned(Job* job, uint32_t task_id, uuid_t peer_id){
    if(!job || task_id >= job->n_tasks)
        return -1;

    pthread_mutex_lock(&job->job_mutex);
    Task* t = &job->tasks[task_id];

    if(t->status != TASK_READY){
        pthread_mutex_unlock(&job->job_mutex);
        return -1; // solo un task READY puo' essere assegnato
    }

    memcpy(t->assigned_to, peer_id, sizeof(uuid_t));
    t->status = TASK_ASSIGNED;

    pthread_mutex_unlock(&job->job_mutex);
    return 0;
}


int job_task_completed(Job* job, uint32_t task_id, void* result, size_t result_len){
    if(!job || task_id >= job->n_tasks)
        return -1;

    pthread_mutex_lock(&job->job_mutex);
    Task* done = &job->tasks[task_id];

    done->result = result;
    done->result_len = result_len;
    done->status = TASK_DONE;
    job->n_done++;

    // sblocca i task che dipendevano da task_id
    for(uint32_t i = 0; i < job->n_tasks; i++){
        Task* t = &job->tasks[i];
        if(t->status != TASK_PENDING) continue;

        for(uint32_t d = 0; d < t->n_deps; d++){
            if(t->deps[d] == task_id){
                t->n_deps_done++;
                break;
            }
        }
        if(t->n_deps_done == t->n_deps)
            t->status = TASK_READY;
    }

    pthread_mutex_unlock(&job->job_mutex);
    return 0;
}


int job_reassign_task(Job* job, uint32_t task_id){
    if(!job || task_id >= job->n_tasks)
        return -1;

    pthread_mutex_lock(&job->job_mutex);
    Task* t = &job->tasks[task_id];

    if(t->status != TASK_ASSIGNED){
        pthread_mutex_unlock(&job->job_mutex);
        return -1;
    }

    memset(t->assigned_to, 0, sizeof(uuid_t));
    t->status = TASK_READY;

    pthread_mutex_unlock(&job->job_mutex);
    return 0;
}


bool job_is_complete(Job* job){
    if(!job) return false;

    pthread_mutex_lock(&job->job_mutex);
    bool complete = (job->n_done == job->n_tasks);
    pthread_mutex_unlock(&job->job_mutex);

    return complete;
}
