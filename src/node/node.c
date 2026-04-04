#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "node.h"

Node setup_node(uint16_t hello_port, uint16_t connection_port, char* hello_multicast) {    
    Node n;
    n.h_port = hello_port;
    n.c_port = connection_port;
    strncpy(n.h_multicast, hello_multicast, INET_ADDRSTRLEN);
    n.h_multicast[INET_ADDRSTRLEN - 1] = '\0';

    printf("setup_node successfull: \nh_port: %d \nc_port: %d \nh_multicast: %s\n",
            n.h_port, n.c_port, n.h_multicast);

    return n;
} 

