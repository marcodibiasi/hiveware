#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>
#include <uuid/uuid.h>
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
	uuid_generate_random(n.id);
    atomic_init(&n.hello_t_running, false);

	init_h_socket(&n);

    print_uuid(n.id);
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

	printf("h_socket created successfully\n");
}


void hello_handler(Node* n){
	atomic_store(&n->hello_t_running, true);

	pthread_t h_send, h_recv; 

	if(pthread_create(&h_send, NULL, send_hello, (void*)n) != 0)
		exit_error("pthread_create h_send");	
	if(pthread_create(&h_recv, NULL, recv_hello, (void*)n) != 0)
		exit_error("pthread_create h_recv");
	

	/* TODO:
	shutdown logic
	unlock recv with shutdown(n->h_socket, SHUT_RDWR);
	*/


	if(pthread_join(h_send, NULL) != 0)
		exit_error("pthread_join h_send");
	if(pthread_join(h_recv, NULL) != 0)
		exit_error("pthread_join h_recv");
}


void* send_hello(void* arg){
	Node *n = (Node*)arg;
	Message msg; 	
    msg.type = HELLO;
    memcpy(msg.node_id, n->id, sizeof(uuid_t));
    
    // sending multicast address 
    struct sockaddr_in mcast_addr = {0};
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_port = htons(n->config->h_port);
    inet_pton(AF_INET, n->config->h_multicast, &mcast_addr.sin_addr);

    // setting up the time spec (conversion from ms) 
    struct timespec ts; 
    ts.tv_sec = n->config->heartbeat_ms / 1000;
    ts.tv_nsec = (n->config->heartbeat_ms % 1000) * 1000000;

	while(atomic_load(&n->hello_t_running)){
        sendto(n->h_socket, &msg, sizeof(msg), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
        printf("HELLO: ");
        print_uuid(msg.node_id);

        nanosleep(&ts, NULL);
	}

	return NULL;
}


void* recv_hello(void* arg){
	Node *n = (Node*)arg;
    Message msg;
        
    while(atomic_load(&n->hello_t_running)){
        struct sockaddr_in src_addr;
        socklen_t addr_len;

        ssize_t msg_size = recvfrom(n->h_socket, &msg, sizeof(msg), 0, 
                (struct sockaddr*)&src_addr, &addr_len);
        if(msg_size == 0) {
            perror("recvfrom");
            continue;
        }

        print_uuid(msg.node_id);
    }

	return NULL;
}
