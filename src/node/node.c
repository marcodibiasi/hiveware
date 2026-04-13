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
#include "peer_disc.h"

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
    Node receiver;
    receiver.config = config; 
	uuid_generate_random(receiver.id);
    atomic_init(&receiver.hello_t_running, false);
    receiver.peer_table = peer_discovery_init(config->timeout_ms);

	init_h_socket(&receiver);

    print_uuid(receiver.id);
    return receiver;
}


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
	addr.sin_port = htons(receiver->config->h_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(receiver->h_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
		exit_error("h_socket bind"); 

	// JOIN MULTICAST 
	struct ip_mreq mreq;
	inet_pton(AF_INET, receiver->config->h_multicast, &mreq.imr_multiaddr); 
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	
	if(setsockopt(receiver->h_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) 
		exit_error("setsockopt IP_ADD_MEMBERSHIP");

	printf("h_socket created successfully\n");
}


void hello_handler(Node* receiver){
	atomic_store(&receiver->hello_t_running, true);

	pthread_t h_send, h_recv; 

	if(pthread_create(&h_send, NULL, send_hello, (void*)receiver) != 0)
		exit_error("pthread_create h_send");	
	if(pthread_create(&h_recv, NULL, recv_hello, (void*)receiver) != 0)
		exit_error("pthread_create h_recv");
	

	/* TODO:
	shutdown logic
	unlock recv with shutdown(receiver->h_socket, SHUT_RDWR);
	*/


	if(pthread_join(h_send, NULL) != 0)
		exit_error("pthread_join h_send");
	if(pthread_join(h_recv, NULL) != 0)
		exit_error("pthread_join h_recv");
}


void* send_hello(void* arg){
	Node *receiver = (Node*)arg;
	Message msg; 	
    msg.type = HELLO;
    memcpy(msg.node_id, receiver->id, sizeof(uuid_t));
    
    // sending multicast address 
    struct sockaddr_in mcast_addr = {0};
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_port = htons(receiver->config->h_port);
    inet_pton(AF_INET, receiver->config->h_multicast, &mcast_addr.sin_addr);

    // setting up the time spec (conversion from ms) 
    struct timespec ts; 
    ts.tv_sec = receiver->config->heartbeat_ms / 1000;
    ts.tv_nsec = (receiver->config->heartbeat_ms % 1000) * 1000000;

	while(atomic_load(&receiver->hello_t_running)){
        sendto(receiver->h_socket, &msg, sizeof(msg), 0, (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));

        nanosleep(&ts, NULL);
	}

	return NULL;
}


void* recv_hello(void* arg){
	Node *receiver = (Node*)arg; /*Ho cambiato il nome da n a receiver*/
    Message msg;
    
    
    while(atomic_load(&receiver->hello_t_running)){
        struct sockaddr_in src_addr;
        socklen_t addr_len;

        ssize_t msg_size = recvfrom(receiver->h_socket, &msg, sizeof(msg), 0, 
                (struct sockaddr*)&src_addr, &addr_len);
        if(msg_size == 0) {
            perror("recvfrom");
            continue;
        }
        /*ANDREA*/
        int index = peer_add(receiver->peer_table, msg.node_id);
        /*
        Qui controlliamo se il nodo è presente, ma peer_daemon potrebbe eliminare in questo istante il nodo. Bisognerebbe
        aggiungere un controllo più robusto, con una variabile globale, ma questo aumenterebbe la complessità. Per adesso
        va bene così ma in futuro non dimentichiamoci di implementare questo tipo di controllo
        */
        if(index == -1){
            printf("No peers available");
            exit_error("No peers");
            /*TODO: immplementare logica di aggiungere spazio nella lista o rimuovere peers inutili*/
        }
        /*------*/

        // DEBUG SECTION
        // printing if the sending node is not the sending node
        if(memcmp(receiver->id, msg.node_id, sizeof(msg.node_id)) != 0) { 
            printf("HELLO from: ");
            print_uuid(msg.node_id);
        }
    }

	return NULL;
}
