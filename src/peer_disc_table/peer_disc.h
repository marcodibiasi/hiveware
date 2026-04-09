#ifndef PEER_DISC_H
#define PEER_DISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <time.h>

#define MAX_PEERS 255


typedef struct{
    bool is_empty;              // This boolean flag tells us if the slot is empty or not
    uuid_t node_id;             // Unique 128bit id; uuid v4
    struct sockaddr_in addr;    // Node address
    struct timespec last_hello; // last_hello sent from the node 

    pthread_mutex_t lock_empty; /*This mutex needs us to lock when we change is_empty's value*/
}DiscNode;

// Forward declaration to avoid include loops
struct Node;
typedef struct{
    DiscNode Node[MAX_PEERS]; /*disc_node*/
    uint32_t timeout; 
}PeerDiscovery;

void update_client_timer(DiscNode* Node); /*Updates timer for each client using cpu clocks*/

PeerDiscovery* peer_discovery_init(); /*Initialize peer_discovery table*/
void peer_discovery_destroy(PeerDiscovery* peer_table); /*Destroy peer table*/
int peer_add(PeerDiscovery* peer_table, uuid_t node_id); /*This function adds a peer in the discovery_table*/
int peer_remove(PeerDiscovery* peer_table, uuid_t node_id); /*This function removes a peer in the discovery table*/
void *peer_daemon(void* arg); /*This function removes peers that do not send the HELLO message*/

#endif
