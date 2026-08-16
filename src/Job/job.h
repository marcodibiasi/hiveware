#ifndef JOB_H
#define JOB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <uuid/uuid.h>

/*
 * Job = calcolo complessivo, scomposto in un DAG di Task.
 *
 * Ogni Task ha task_id sequenziale (0..n_tasks-1), usato anche come
 * indice diretto nell'array job->tasks (niente ricerca lineare).
 *
 * Stati di un Task:
 *   PENDING  -> in attesa che tutte le sue dipendenze completino
 *   READY    -> dipendenze risolte, pronto per essere assegnato a un peer
 *   ASSIGNED -> inviato a un peer, in attesa del risultato
 *   DONE     -> risultato ricevuto
 *
 * Se il peer assegnato sparisce prima di rispondere, job_reassign_task()
 * riporta il task a READY cosi' puo' essere assegnato a un altro nodo.
 */

typedef enum {
    TASK_PENDING,
    TASK_READY,
    TASK_ASSIGNED,
    TASK_DONE
} TaskStatus;

typedef struct {
    uint32_t task_id;
    uint32_t* deps;         // task_id delle dipendenze
    uint32_t n_deps;
    uint32_t n_deps_done;   // quante dipendenze sono gia' TASK_DONE

    TaskStatus status;
    uuid_t assigned_to;     // node_id del peer a cui e' stato assegnato

    void* payload;          // dati di input (es. blocco di matrice)
    size_t payload_len;

    void* result;           // dati di output, valido quando status == TASK_DONE
    size_t result_len;
} Task;

typedef struct {
    uuid_t job_id;
    Task* tasks;             // array di n_tasks elementi
    uint32_t n_tasks;
    uint32_t n_done;
    pthread_mutex_t job_mutex;
} Job;

Job* job_create(uint32_t n_tasks); /*Alloca un job con n_tasks task vuoti, tutti PENDING*/
void job_destroy(Job* job);

/* Popola il task task_id: copia deps (n_deps elementi) e il payload.
 * Se n_deps == 0 il task passa subito a READY.
 * Ownership di payload passa al job (verra' liberato da job_destroy). */
int job_set_task(Job* job, uint32_t task_id, uint32_t* deps, uint32_t n_deps,
                  void* payload, size_t payload_len);

/* Ritorna un array malloc'd di task_id in stato READY (*out_count elementi).
 * Non cambia stato: chiamare job_mark_assigned() su ognuno dopo l'invio. */
uint32_t* job_get_ready_tasks(Job* job, uint32_t* out_count);

int job_mark_assigned(Job* job, uint32_t task_id, uuid_t peer_id);

/* Registra il risultato di task_id, lo marca DONE e sblocca i dipendenti
 * la cui ultima dipendenza pendente era proprio task_id.
 * Ownership di result passa al job. */
int job_task_completed(Job* job, uint32_t task_id, void* result, size_t result_len);

/* Il peer assegnato a task_id e' morto prima di rispondere: torna READY. */
int job_reassign_task(Job* job, uint32_t task_id);

bool job_is_complete(Job* job);

#endif // JOB_H
