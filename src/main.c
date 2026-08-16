#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "utils.h"
#include "node.h"
#include "peer_disc.h"
#include "tcp.h"
#include "job.h"
#include "job_registry.h"

// Finto executor che rispetta la tua firma esatta per 'TaskExecutor'
int dummy_executor(const void* payload, size_t payload_len, void** result, size_t* result_len) {
    printf("\n>>> [WORKER] Ho ricevuto un task da eseguire!\n");
    printf(">>> [WORKER] Payload ricevuto: '%s'\n", (const char*)payload);
    
    sleep(2); // Simula 2 secondi di elaborazione intensa
    
    const char* res_str = "RISPOSTA: Computazione completata con successo!";
    *result_len = strlen(res_str) + 1;
    *result = malloc(*result_len);
    
    if(*result) {
        memcpy(*result, res_str, *result_len);
    }
    
    printf(">>> [WORKER] Risultato generato. Il protocollo TCP lo spedira' indietro!\n\n");
    return 0; // 0 = OK (un valore != 0 innescherebbe MSG_TASK_ERROR)
}

int main(int argc, char* argv[]) {
    NodeConfig nconfig = default_nodeconfig();
    bool is_coordinator = false; // Flag per decidere se fare la parte attiva o passiva

    if(argc > 1){
        int port = atoi(argv[1]);
        if(port <= 0 || port > 65535){
            fprintf(stderr, "porta non valida: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
        nconfig.c_port = (uint16_t)port;
        is_coordinator = true;
        printf("c_port sovrascritta da riga di comando: %d\n", nconfig.c_port);
    }

    Node node = init_node(nconfig);
    
    // 1. Assegniamo l'executor PRIMA del thread daemon tcp (start_node)
    node.task_executor = dummy_executor; 
    
    start_node(&node);

    // ==========================================
    // -- TEST E2E: COORDINATORE --
    // ==========================================
    if (is_coordinator) {
        printf("\n[TEST] In attesa di scoprire il Nodo Worker nella rete UDP...\n");
        DiscNode target_node;
        bool peer_found = false;

        while (!peer_found && atomic_load(&node.running)) {
            pthread_mutex_lock(&node.peer_table->table_mutex);
            if (node.peer_table->n_nodes > 0) {
                target_node = node.peer_table->Node[0]; // Prendiamo il primo nodo
                peer_found = true;
            }
            pthread_mutex_unlock(&node.peer_table->table_mutex);
            if (!peer_found) sleep(1);
        }

        if (peer_found && atomic_load(&node.running)) {
            char uuid_str[37];
            uuid_unparse(target_node.node_id, uuid_str);
            printf("\n[TEST] Worker %s scoperto! (TCP Porta %d). Preparazione Job...\n", uuid_str, target_node.c_port);

            Job* test_job = job_create(1);
            char* payload = strdup("DATI_FITTIZI_DA_ELABORARE");
            
            // Il job si prende l'ownership del payload e lo libera in job_destroy
            job_set_task(test_job, 0, NULL, 0, payload, strlen(payload) + 1);
            job_registry_add(node.job_registry, test_job);
            job_mark_assigned(test_job, 0, target_node.node_id);

            // 2. Connessione TCP Reale
            printf("[TEST] Apertura socket TCP...\n");
            int fd = tcp_connect_to_peer(&node, &target_node);
            
            if (fd >= 0) {
                printf("[TEST] Frame MSG_TASK_ASSIGN in invio...\n");
                tcp_send_task_assign(fd, test_job->job_id, 0, payload, strlen(payload) + 1);

                // 3. Attendiamo il risultato sullo STESSO socket
                printf("[TEST] In attesa asincrona del MSG_TASK_RESULT sul socket...\n");
                MessageType type;
                void* body = NULL;
                uint32_t body_len = 0;
                
                if (tcp_recv_frame(fd, &type, &body, &body_len) == 0) {
                    if (type == MSG_TASK_RESULT && body_len >= sizeof(TaskFrameHeader)) {
                        // Estraiamo gli Header (job_id e task_id)
                        TaskFrameHeader tfh;
                        memcpy(&tfh, body, sizeof(tfh));
                        
                        // Estraiamo il vero risultato e lo allochiamo per il Job
                        size_t res_len = body_len - sizeof(TaskFrameHeader);
                        void* res_payload = malloc(res_len);
                        if (res_payload) {
                            memcpy(res_payload, (uint8_t*)body + sizeof(TaskFrameHeader), res_len);
                            
                            // 4. Lo registriamo come completato nel Job
                            Job* j = job_registry_find(node.job_registry, tfh.job_id);
                            if(j) job_task_completed(j, tfh.task_id, res_payload, res_len);
                        }
                    } else if (type == MSG_TASK_ERROR) {
                        printf("[TEST FALLITO] Ricevuto un frame MSG_TASK_ERROR dal worker!\n");
                    }
                    free(body); // Il body base di recv_frame va liberato manualmente dal chiamante
                }
                close(fd); 
            } else {
                printf("[TEST FALLITO] Connessione TCP verso il Worker fallita.\n");
            }

            // Verifica Finale
            if (job_is_complete(test_job)) {
                printf("\n============================================\n");
                printf("[TEST SUPERATO] E' arrivata la risposta dal Worker!\n");
                printf("[RISULTATO]: %s\n", (char*)test_job->tasks[0].result);
                printf("============================================\n\n");
            }

            // Pulizia
            job_registry_remove(node.job_registry, test_job->job_id);
            job_destroy(test_job);
        }
    } else {
        printf("\n[WORKER] Avviato come passivo sulla porta %d. In attesa di calcoli...\n", nconfig.c_port);
    }

    pause(); // Il nodo continua a girare finché non chiudi il processo
    stop_node(&node);

    return EXIT_SUCCESS;
}