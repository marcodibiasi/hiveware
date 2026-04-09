#ifndef PEER_DISC_H
#define PEER_DISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include "node.h"


#define MAX_PEERS 255


typedef struct Disc_Node{
    bool is_empty;              // This boolean flag tells us if the slot is empty or not
    uuid_t node_id;             // Unique 128bit id; uuid v4
    struct sockaddr_in addr;    // Node address
}DiscNode;


typedef struct PeerDiscovery{
    DiscNode Node[MAX_PEERS]; /*disc_node*/
}PeerDiscovery;

uint8_t peer_add(PeerDiscovery* peer_table, uuid_t node_id); /*This function adds a peer in the discovery_table*/
uint8_t peer_remove(PeerDiscovery* peer_table, uuid_t node_id); /*This function removes a peer in the discovery table*/

#endif
