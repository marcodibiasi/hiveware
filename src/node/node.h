#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <uuid/uuid.h>
#include "peer_disc.h" 

/*
Node stores the execution variables to setup before running the nodes. 
h_port is a UDP port for the HELLO phase. 
c_port is a TCP port to ensure the connection to the peers after the discovery. 
h_multicast is a multicast ip used to send a HELLO message to the LAN. 
*/

typedef enum {
	HELLO
} Type;

typedef struct {
    char h_multicast[INET_ADDRSTRLEN];  // h_multicast is a multicast ip used to send a HELLO message to the LAN. 
    uint16_t h_port; 			        // h_port is a UDP port for the HELLO phase. 
	uint16_t c_port; 			        // c_port is a TCP port to ensure the connection to the peers after the discovery.

    uint32_t heartbeat_ms;             
    uint32_t timeout_ms; 
} NodeConfig;

typedef struct {
    uuid_t id;  // uuid v4 128bit identifier
    PeerDiscovery* peer_table;    

    const NodeConfig* config;
    int h_socket;
    int c_socket;

    atomic_bool hello_t_running; 
} Node;

#pragma pack(push, 1)
typedef struct {
    uuid_t node_id;
	Type type;
} Message;
#pragma pack(pop)

NodeConfig default_nodeconfig(void);
Node init_node(const NodeConfig* config);
void init_h_socket(Node* n);
void hello_handler(Node* n);
void* send_hello(void* arg);
void* recv_hello(void* arg);

#endif // NODE_H
