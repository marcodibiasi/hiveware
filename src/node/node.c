#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "node.h"
#include "utils.h"

NodeConfig default_nodeconfig(void) {
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

	init_h_socket(&n);
    
    return n;
}


void init_h_socket(Node *n){

	if((n->h_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        exit_error("h_socket");  

	int opt = 1;
	if(setsockopt(n->h_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		exit_error("setsockopt SO_REUSEADDR");

	// BIND
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(n->config->h_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(n->h_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
		exit_error("h_socket bind"); 

	// JOIN MULTICAST 
	struct ip_mreq mreq;
	inet_pton(AF_INET, n->config->h_multicast, &mreq.imr_multiaddr); 
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	
	if(setsockopt(n->h_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) 
		exit_error("setsockopt IP_ADD_MEMBERSHIP");

	printf("h_socket created successfully");
}
