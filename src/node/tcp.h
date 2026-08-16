#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>
#include <uuid/uuid.h>
#include "peer_disc.h" // per DiscNode

typedef struct Node Node; // forward declaration, come in udp.h

/*
 * Framing dei messaggi sul socket TCP:
 *
 *   [ MAGIC(4) | VERSION(1) | MSG_TYPE(1) | PAYLOAD_LEN(4) | PAYLOAD(N) ]
 *
 * PAYLOAD_LEN e' la lunghezza in byte di cio' che segue l'header.
 * Per TASK_ASSIGN/TASK_RESULT/TASK_ERROR il payload inizia sempre con
 * un TaskFrameHeader (job_id + task_id), seguito dai dati veri e propri
 * (assenti per TASK_ERROR).
 */

#define HIWA_MAGIC 0x4849574Au // "HIWA"
#define HIWA_PROTO_VERSION 1
#define TCP_BACKLOG 16
#define MAX_FRAME_PAYLOAD (64u * 1024 * 1024) // guardia anti payload_len corrotto/malevolo

typedef enum {
    MSG_TASK_ASSIGN = 1, // coordinator -> peer: "esegui questo task"
    MSG_TASK_RESULT = 2, // peer -> coordinator: "ecco il risultato"
    MSG_TASK_ERROR  = 3  // peer -> coordinator: "non sono riuscito a eseguirlo"
} MessageType;

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t msg_type;
    uint32_t payload_len;
} FrameHeader;

typedef struct {
    uuid_t job_id;
    uint32_t task_id;
} TaskFrameHeader;
#pragma pack(pop)

/*
 * Funzione che esegue davvero un task ricevuto: riceve payload/payload_len,
 * deve allocare *result (malloc) e impostare *result_len, ritorna 0 se ok.
 * Il chiamante (tcp_conn_handler) fa la free(*result) dopo l'invio.
 * Se node->task_executor e' NULL, ogni TASK_ASSIGN ricevuto viene rifiutato
 * con TASK_ERROR: serve per testare il trasporto prima di avere calcoli veri.
 */
typedef int (*TaskExecutor)(const void* payload, size_t payload_len,
                             void** result, size_t* result_len);

// --- framing di basso livello ---
int tcp_send_frame(int sockfd, MessageType type, const void* body, uint32_t body_len);
int tcp_recv_frame(int sockfd, MessageType* type, void** body, uint32_t* body_len); // *body va liberato dal chiamante

// --- helper sui tre tipi di messaggio ---
int tcp_send_task_assign(int sockfd, const uuid_t job_id, uint32_t task_id,
                          const void* payload, size_t payload_len);
int tcp_send_task_result(int sockfd, const uuid_t job_id, uint32_t task_id,
                          const void* result, size_t result_len);
int tcp_send_task_error(int sockfd, const uuid_t job_id, uint32_t task_id);

// --- connection manager ---
void init_c_socket(Node* node);      // crea/bind/listen sulla porta c_port
void* tcp_accept_daemon(void* arg);  // Node*, accetta connessioni e spawna un handler per ciascuna
void* tcp_conn_handler(void* arg);   // gestisce una singola connessione TCP, dispatcha i frame

/* Apre una connessione TCP verso un peer del peer_table, usando la sua
 * c_port reale (annunciata via HELLO), non quella locale di questo nodo. */
int tcp_connect_to_peer(Node* node, DiscNode* peer);

#endif
