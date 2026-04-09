#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <uuid/uuid.h>
#include "peer_disc.h"
#include "utils.h"
#include <time.h>
#include <pthread.h>

#define TIMEOUT_SECONDS 10

PeerDiscovery* peer_discovery_init(){
    
    PeerDiscovery* pt = calloc(1, sizeof(PeerDiscovery));
    if(!pt) exit_error("Calloc Failed");

    for(int i = 0; i<MAX_PEERS;i++){
        pt->Node[i].is_empty = true;
        if(pthread_mutex_init(&pt->Node[i].lock_empty,NULL) != 0){
            for(int j = 0; j < i; j++)
            pthread_mutex_destroy(&pt->Node[j].lock_empty);
            free(pt);
            exit_error("pthread_mutex_init failed");

        }
    }
    return pt;

}

void peer_discovery_destroy(PeerDiscovery* peer_table){
    
    if(!peer_table) return;

    for(int i = 0; i<MAX_PEERS;i++){
        pthread_mutex_destroy(&peer_table -> Node[i].lock_empty);
    }
    
    free(peer_table);

}

uint8_t peer_add(PeerDiscovery* peer_table, uuid_t node_id){

    if(!peer_table)
        exit_error("peer_table does not exist");

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        pthread_mutex_lock(&peer_table -> Node[i].lock_empty);
        if(peer_table->Node[i].is_empty) {
            peer_table->Node[i].is_empty = false;
            pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);
            memcpy(peer_table->Node[i].node_id, node_id, sizeof(uuid_t));
            return 0; // success
        }
        pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);
    }
    return -1; /*No peers available*/
}


uint8_t peer_remove(PeerDiscovery* peer_table, uuid_t node_id){
    
    if(!peer_table)
        exit_error("peer_table does not exist");

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        if(memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0) {
            pthread_mutex_lock(&peer_table->Node[i].lock_empty);
            peer_table -> Node[i].is_empty = true;
            pthread_mutex_unlock(&peer_table->Node[i].lock_empty);
            return 0; // Found 
        }
    }
    return 1; // node_id not found
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

                pthread_mutex_lock(&peer_table -> Node[i].lock_empty);
                peer_table -> Node[i].is_empty = true;
                pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);

            }
        }
}
return NULL;
}