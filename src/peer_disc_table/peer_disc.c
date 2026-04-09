#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "peer_disc.h"
#include "utils.h"
#include <time.h>
#include <pthread.h>

#define TIMEOUT_SECONDS 10

uint8_t peer_add(PeerDiscovery* peer_table, uint8_t* mac_address){

    if(!peer_table){
        exit_error("Peer table is empty");
        return 1;
    }

    if(!mac_address){
        exit_error("Mac address does not exist or length is less than 6 bytes");
        return 2;
    }

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        pthread_mutex_lock(&peer_table -> Node[i].lock_empty);
        if(peer_table -> Node[i].is_empty)
            {
                peer_table -> Node[i].is_empty = false;
                pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);
                memcpy(peer_table->Node[i].mac_address, mac_address, 6);
                peer_table->Node[i].arrival_time = peer_table->global_timer;
                peer_table->Node[i].snap_global_timer = peer_table->global_timer;
                return 0;
            }
        pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);
        }
   
    return 3; /*No peers available*/
}


uint8_t peer_remove(PeerDiscovery* peer_table, uint8_t* mac_address){
    
    if(!peer_table){
        fprintf(stderr,"Peer table empty");
        return 1;
    }

    if(!mac_address && sizeof(mac_address) < 6){
        fprintf(stderr, "Mac address does not exist or length is less than 6 bytes");
        return 2;
    }

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        if(mac_compare(peer_table -> Node[i].mac_address, mac_address)) /*If true, just remove it*/
            {
                peer_table -> Node[i].is_empty = true;
                return 0; /*Found !*/
            }

    }
    return 3; /*Mac address not found*/   
}

void update_client_timer(DiscNode* Node){
    clock_gettime(CLOCK_MONOTONIC, &Node->last_hello);
}

void *peer_daemon(void* arg){
    /*This function controls that each client has been active in the last 10 seconds
      else, it removes the client from the peer_table
    */
    PeerDiscovery* peer_table = (PeerDiscovery* ) arg;
    while(1){
        sleep(2);
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        for(int i = 0; i < MAX_PEERS;i++){
            pthread_mutex_lock(&peer_table -> Node[i].lock_empty);
            bool empty = peer_table -> Node[i].is_empty;
            pthread_mutex_unlock(&peer_table->Node[i].lock_empty); /*This variable MUST be protected by a mutex*/

            if(empty) continue;
            /*Elapsed controls if the now time minus arrival time is greater than TIMEOUT_SECONDS*/
            double elapsed =
                (now.tv_sec - peer_table -> Node[i].last_hello.tv_sec)+
                (now.tv_nsec - peer_table -> Node[i].last_hello.tv_nsec) / 1e9;

            if (elapsed > TIMEOUT_SECONDS){
                printf("Client expired, node number %d has been removed\n",i);

                pthread_mutex_lock(&peer_table -> Node[i].lock_empty)
                peer_table -> Node[i].is_empty = true;
                pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);

            }
        }
}
return NULL;
}