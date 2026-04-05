#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "node.h"

void exit_error(char* msg){
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}


NodeConfig default_nodeconfig() {
    NodeConfig c; 
    strcpy(c.h_multicast, "239.255.0.1");
    c.h_port = 50000;
    c.c_port = 50001;
    c.heartbeat_ms = 2000;
    c.timeout_ms = 8000;

    printf("default nodeconfig created: \nh_port: %d" 
            "\nc_port: %d \nh_multicast: %s"
            "\nheartbeat_ms = %d \ntimeout_ms = %d\n",
            c.h_port, c.c_port, c.h_multicast, c. heartbeat_ms, c.timeout_ms);

    return c;
}


Node init_node(const NodeConfig* config) {    
    Node n;
    n.config = config; 
    
    if((n.h_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
        exit_error("h_socket");  

    if((n.c_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        exit_error("h_socket");  
  
    printf("socket created successfully\n");
    return n;
}


void* heartbeat_hello(void* arg) {
    return NULL;
}
