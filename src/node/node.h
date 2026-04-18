#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <uuid/uuid.h>
#include <pthread.h>
#include "peer_disc.h" 
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
} NodeThreads;

typedef struct Node{
    uuid_t id;  // uuid v4 128bit identifier
    PeerDiscovery* peer_table;    
    NodeThreads nthreads;

    NodeConfig config; // Do not modify in threads
    int h_socket;
    int c_socket;

    atomic_bool running; 
} Node;

#pragma pack(push, 1)
typedef struct {
    uuid_t node_id;
	Type type;
    uv_cpu_info_t* cpu_info;
} Message;
#pragma pack(pop)

NodeConfig default_nodeconfig(void);
void load_or_create_uuid(uuid_t id);
Node init_node(NodeConfig config);
void start_node(Node* node);
void stop_node(Node* node);

#endif // NODE_H
