#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include "peer_disc.h" 
#include "client_info.h"
#include "job_registry.h"
#include "tcp.h"
#include "uv.h"

/*
 * Node configuration:
 * - h_port:       UDP port for discovery (HELLO)
 * - c_port:       TCP listening port (tasks/data)
 * - h_multicast:  multicast address for HELLO
 * - heartbeat_ms: interval between HELLO messages
 * - timeout_ms:   peer removal timeout (peer_table)
 */

/*
 * Messages:
 * - HELLO: UDP multicast discovery message
 *          includes node ID and uv_cpu_info_t
 */

typedef enum {
	HELLO
} Type;

typedef struct {
    char h_multicast[INET_ADDRSTRLEN];  
    uint16_t h_port; 			        
	uint16_t c_port; 			        

    uint32_t heartbeat_ms;             
    uint32_t timeout_ms; 
} NodeConfig;

typedef struct {
    pthread_t peer_table_daemon, print_table_daemon;
    pthread_t h_send, h_recv; 
    pthread_t tcp_accept;
} NodeThreads;

typedef struct Node{
    uuid_t id;  // uuid v4 128bit identifier
    PeerDiscovery* peer_table;    
    NodeThreads nthreads;

    NodeConfig config; // Do not modify in threads
    int h_socket;
    int c_socket;

    // Capacita' locale (calcolata una volta all'avvio) e ultimo
    // campionamento dei cpu_times, usato da send_hello per calcolare
    // il delta di carico da allegare al prossimo HELLO. Toccato solo
    // dal thread h_send: nessuna sincronizzazione necessaria.
    uint16_t n_cores;
    uint16_t avg_mhz;
    CpuTimesSnapshot last_cpu_snapshot;

    JobRegistry* job_registry;   // job attivi di cui questo nodo e' coordinator
    TaskExecutor task_executor;  // funzione che esegue i task ricevuti; NULL = rifiuta tutto (vedi tcp.h)

    atomic_bool running; 
} Node;

/*
 * HELLO piggyback: ogni nodo allega un sommario delle proprie risorse
 * e la propria porta TCP reale (necessaria perche' tcp_connect_to_peer
 * non puo' piu' assumere che tutti i nodi usino la stessa c_port).
 * - n_cores/avg_mhz: capacita', quasi statica (ricalcolata di rado)
 * - load_pct:        carico corrente 0-100, ricalcolato ad ogni HELLO
 *   come delta dei cpu_times tra due invii successivi (vedi udp.c)
 */
#pragma pack(push, 1)
typedef struct {
    uuid_t node_id;
	Type type;
    uint16_t n_cores;
    uint16_t avg_mhz;
    uint8_t load_pct;
    uint16_t c_port; // porta TCP su cui questo nodo ascolta davvero
} Message;
#pragma pack(pop)

NodeConfig default_nodeconfig(void);
void load_or_create_uuid(uuid_t id);
Node init_node(NodeConfig config);
void start_node(Node* node);
void stop_node(Node* node);

#endif // NODE_H
