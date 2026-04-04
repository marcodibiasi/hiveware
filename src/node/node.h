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

typedef struct node{
    uint16_t h_port;
    uint16_t c_port;
    char h_multicast[INET_ADDRSTRLEN];
} Node;

Node setup_node(uint16_t hello_port, uint16_t connection_port, char* hello_multicast);

#endif // NODE_H
