#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "peer_disc.h"
#include "../utils/utils.h"


uint8_t peer_add(PeerDiscovery* Peer_table, uint8_t* mac_address){

    if(!Peer_table){
        exit_error("Peer table is empty");
        return 1;
    }

    if(!mac_address){
        exit_error("Mac address does not exist or length is less than 6 bytes");
        return 2;
    }

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        if(Peer_table -> Node[i].is_empty)
            {
                Peer_table -> Node[i].is_empty = false;
                memcpy(Peer_table->Node[i].mac_address,mac_address,6);
                return 0;
            }
        }
   
    return 3; /*No peers available*/

}

uint8_t peer_remove(PeerDiscovery* Peer_table, uint8_t* mac_address){
    
    if(!Peer_table){
        fprintf(stderr,"Peer table empty");
        return 1;
    }

    if(!mac_address && sizeof(mac_address) < 6){
        fprintf(stderr, "Mac address does not exist or length is less than 6 bytes");
        return 2;
    }

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        if(mac_compare(Peer_table -> Node[i].mac_address, mac_address)) /*If true, just remove it*/
            {
                Peer_table -> Node[i].is_empty = true;
                return 0; /*Found !*/
            }

    }
    return 3; /*Mac address not found*/   
}
