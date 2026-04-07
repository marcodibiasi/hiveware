#ifndef PEER_DISC_H
#define PEER_DISC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../node/node.h"


#define MAX_PEERS 255


typedef struct Disc_Node{

    bool is_empty; /*This boolean flag tells us if the node is empty or not: False if it is not empty, True otherwise*/
    uint8_t mac_address[6]; /*mac address is 6 bytes long.*/
    uint32_t timer; /*A simple timer.*/

}DiscNode;


typedef struct Peer_Discovery{
    
    DiscNode Node; /*disc_node*/

}PeerDiscovery;

void peer_add(PeerDiscovery* Peer_table, uint8_t* mac_address); /*This function adds a peer in the discovery_table*/
void peer_remove(PeerDiscovery* Peer_table, uint8_t* mac_address); /*This function removes a peer in the discovery table*/

#endif