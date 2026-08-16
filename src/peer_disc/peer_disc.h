#ifndef PEER_DISC_H
#define PEER_DISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <time.h>
#include "uv.h"

#define MAX_PEERS 64


typedef struct{
    uint64_t user;
    uint64_t sys;
    uint64_t idle;
    uint64_t nice;
    uint64_t irq;
} cpu_times_t;


// This new version remove the local mutex appraoch for a global mutex 
typedef struct{
    uuid_t node_id;             // Unique 128bit id; uuid v4
    struct sockaddr_in addr;    // Node address
    struct timespec last_hello; // monotonic time since last_hello was sent
    double elapsed;             // delta time (now - last_hello); sake of visualization
    
    int cpu_count;
    uv_cpu_info_t* cpu;                                 
    cpu_times_t* delta_times;
}DiscNode;

typedef struct{
    DiscNode Node[MAX_PEERS]; /*disc_node*/
    uint32_t timeout; 
    int n_nodes;

    pthread_mutex_t table_mutex;
}PeerDiscovery;

void update_client_timer(DiscNode* Node); /*Updates timer for each client using cpu clocks*/
int is_inside(PeerDiscovery* peer_table, uuid_t node_id);
PeerDiscovery* peer_discovery_init(uint32_t timeout); /*Initialize peer_discovery table*/
void peer_discovery_destroy(PeerDiscovery* peer_table); /*Destroy peer table*/
int peer_add(PeerDiscovery* peer_table, uuid_t node_id, struct sockaddr_in addr); /*This function adds a peer in the discovery_table*/
int peer_remove(PeerDiscovery* peer_table, uuid_t node_id); /*This function removes a peer in the discovery table*/
void peer_remove_index(PeerDiscovery* peer_table, int index);
void* peer_daemon(void* arg); /*This function removes peers that do not send the HELLO message*/
void* print_daemon(void* arg);
void print_peer_table(PeerDiscovery* pt); 

#endif
