#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <uuid/uuid.h>
#include "node.h"
#include "udp.h"
#include "utils.h"
#include "peer_disc.h"


void init_h_socket(Node *receiver){
	if((receiver->h_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        exit_error("h_socket");  

	int opt = 1;
	if(setsockopt(receiver->h_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
		exit_error("setsockopt SO_REUSEADDR");
    if(setsockopt(receiver->h_socket, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt)) < 0)
        exit_error("setsockopt SO_REUSEPORT");

	// BIND
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(receiver->config.h_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(receiver->h_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
		exit_error("h_socket bind"); 

	// JOIN MULTICAST 
	struct ip_mreq mreq;
	inet_pton(AF_INET, receiver->config.h_multicast, &mreq.imr_multiaddr); 
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	
	if(setsockopt(receiver->h_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) 
		exit_error("setsockopt IP_ADD_MEMBERSHIP");

	printf("h_socket created successfully\n");
}


void* send_hello(void* arg){
	Node *receiver = (Node*)arg;
	Message msg; 	
    msg.type = HELLO;
    memcpy(msg.node_id, receiver->id, sizeof(uuid_t));
    
    // sending multicast address 
    struct sockaddr_in mcast_addr = {0};
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_port = htons(receiver->config.h_port);
    inet_pton(AF_INET, receiver->config.h_multicast, &mcast_addr.sin_addr);

    // setting up the time spec (conversion from ms) 
    struct timespec ts; 
    ts.tv_sec = receiver->config.heartbeat_ms / 1000;
    ts.tv_nsec = (receiver->config.heartbeat_ms % 1000) * 1000000;

	while(atomic_load(&receiver->running)){
        sendto(receiver->h_socket, &msg, sizeof(msg), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));

        nanosleep(&ts, NULL);
	}

	return NULL;
}


void* recv_hello(void* arg){
	Node *receiver = (Node*)arg; 
    Message msg;
    
    while(atomic_load(&receiver->running)){
        struct sockaddr_in src_addr = {0};
        socklen_t addr_len = sizeof(src_addr);

        ssize_t msg_size = recvfrom(receiver->h_socket, &msg, sizeof(msg), 0, 
                (struct sockaddr*)&src_addr, &addr_len);
        if(msg_size < 0) {
            perror("recvfrom");
            continue;
        }
        
        /*
        * filtering self messages
        * In dev phase it can be disabled to debug
        * The filter should be kept for better functioning
        
        if(memcmp(msg.node_id, receiver->id, sizeof(uuid_t)) == 0)
            continue;
        */

        int index = peer_add(receiver->peer_table, msg.node_id, src_addr);

        if(index == -1)
            exit_error("peer_table out of slots");
        
    }

	return NULL;
}
