#ifndef PEER_DISC_H
#define PEER_DISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "node.h"
#include <time.h>

#define MAX_PEERS 255


typedef struct Disc_Node{

    bool is_empty; /*This boolean flag tells us if the node is empty or not: False if it is not empty, True otherwise*/
    uint8_t mac_address[6]; /*mac address is 6 bytes long.*/
    struct timespec last_hello; /*last_hello sent from the client*/

    pthread_mutex_t lock_empty; /*This mutex needs us to lock when we change is_empty's value*/
}DiscNode;

typedef struct Peer_Discovery{

    DiscNode Node[MAX_PEERS]; /*disc_node*/

}PeerDiscovery;

void update_client_timer(PeerDiscovery* peer_table); /*Updates timer for each client using cpu clocks*/

uint8_t peer_add(PeerDiscovery* peer_table, uint8_t* mac_address); /*This function adds a peer in the discovery_table*/
uint8_t peer_remove(PeerDiscovery* peer_table, uint8_t* mac_address); /*This function removes a peer in the discovery table*/
void peer_daemon(PeerDiscovery* peer_table, void* arg); /*This function removes peers that do not send the HELLO message*/

#endif
