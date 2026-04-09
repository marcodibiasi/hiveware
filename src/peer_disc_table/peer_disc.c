#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <uuid/uuid.h>
#include "peer_disc.h"
#include "utils.h"


uint8_t peer_add(PeerDiscovery* peer_table, uuid_t node_id){

    if(!peer_table)
        exit_error("peer_table does not exist");

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        if(peer_table->Node[i].is_empty) {
            peer_table->Node[i].is_empty = false;
            memcpy(peer_table->Node[i].node_id, node_id, sizeof(uuid_t));
            return 0; // success
        }
    }
    return -1; /*No peers available*/
}


uint8_t peer_remove(PeerDiscovery* peer_table, uuid_t node_id){
    
    if(!peer_table)
        exit_error("peer_table does not exist");

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        if(memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0) {
            peer_table -> Node[i].is_empty = true;
            return 0; // Found 
        }
    }
    return -1; // node_id not found
}
