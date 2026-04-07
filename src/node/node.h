#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <netinet/in.h>

/*
Node stores the execution variables to setup before running the nodes. 
h_port is a UDP port for the HELLO phase. 
c_port is a TCP port to ensure the connection to the peers after the discovery. 
h_multicast is a multicast ip used to send a HELLO message to the LAN. 
*/

typedef struct {
    char h_multicast[INET_ADDRSTRLEN]; /*h_multicast is a multicast ip used to send a HELLO message to the LAN. */
    uint16_t h_port;  /*h_port is a UDP port for the HELLO phase. */
    uint16_t c_port; /*c_port is a TCP port to ensure the connection to the peers after the discovery.*/

    uint32_t heartbeat_ms; 
    uint32_t timeout_ms; 
} NodeConfig;

typedef struct {
    const NodeConfig *config;
    int h_socket;
    int c_socket;
} Node;

void exit_error(char* msg);
NodeConfig default_nodeconfig();
Node init_node(const NodeConfig* config);
void* heartbeat_hello(void* arg);

#endif // NODE_H
