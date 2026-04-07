#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "peer_disc.h"

uint8_t peer_add(PeerDiscovery* Peer_table, uint8_t* mac_address){

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
        if(Peer_table -> Node.is_empty)
            {
                Peer_table -> Node.is_empty = false;
                for(int j = 0; j<6; j++){
                    Peer_table-> Node.mac_address[j] = mac_address[j];
                }
            }

    }   

}

void peer_remove(PeerDiscovery* Peer_table, uint8_t* mac_address){
    
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
        if(Peer_table -> Node.is_empty)
            {
                Peer_table -> Node.is_empty = true;
            }

    }   
}
