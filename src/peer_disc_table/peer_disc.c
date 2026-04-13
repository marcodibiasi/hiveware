#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "peer_disc.h"
#include "utils.h"
#include <time.h>
#include <pthread.h>


PeerDiscovery* peer_discovery_init(uint32_t timeout){
    PeerDiscovery* pt = calloc(1, sizeof(PeerDiscovery));
    if(!pt) exit_error("Calloc Failed");

    pt->timeout = timeout;
    
    // Initialize each mutex
    for(int i = 0; i<MAX_PEERS;i++){
        pt->Node[i].is_empty = true;
        if(pthread_mutex_init(&pt->Node[i].lock_empty,NULL) != 0){
            for(int j = 0; j < i; j++) {
                pthread_mutex_destroy(&pt->Node[j].lock_empty);
            }
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

int is_inside(PeerDiscovery* peer_table, uuid_t node_id){

    for(int i = 0; i < MAX_PEERS; i++){
        pthread_mutex_lock(&peer_table->Node[i].lock_empty);
        
        bool empty = peer_table->Node[i].is_empty;
        bool match = memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0;
        
        pthread_mutex_unlock(&peer_table->Node[i].lock_empty);
        
        if(!empty && match) return i;
    }
    return -1;
}

int peer_add(PeerDiscovery* peer_table, uuid_t node_id){
    if(!peer_table)
        exit_error("peer_table does not exist");

    int index = is_inside(peer_table,node_id);

    if(index != -1){
        update_client_timer(&peer_table->Node[index]);
        return index;
    }

    for(int i = 0; i<MAX_PEERS; i++){
        /*If we find an empty node, we just add the mac_address and change the boolean flag to True*/
        pthread_mutex_lock(&peer_table -> Node[i].lock_empty);
        if(peer_table->Node[i].is_empty) {
            peer_table->Node[i].is_empty = false;
            memcpy(peer_table->Node[i].node_id, node_id, sizeof(uuid_t));
            update_client_timer(&peer_table->Node[i]);
            pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);
            return i; // success
        }
        pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);
    }
    return -1; /*No peers available*/
}


int peer_remove(PeerDiscovery* peer_table, uuid_t node_id){
    if(!peer_table)
        exit_error("peer_table does not exist");

    for(int i = 0; i < MAX_PEERS; i++){
        /*If we find an empty node, we just add the uuid and change the boolean flag to True*/
        if(memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0) {
            if(peer_table->Node[i].is_empty == true){
                return 1; /*was already empty*/
            }
            pthread_mutex_lock(&peer_table->Node[i].lock_empty);
            peer_table -> Node[i].is_empty = true;
            pthread_mutex_unlock(&peer_table->Node[i].lock_empty);
            return 0; // Found 
        }
    }
    return -1; // node_id not found
}


void update_client_timer(DiscNode* Node){
    clock_gettime(CLOCK_MONOTONIC, &Node->last_hello);
}


void* peer_daemon(void* arg){
    /*This function checks whether a node has reached the time out 
      if so, it removes the node from the peer_table
    */
    PeerDiscovery* peer_table = (PeerDiscovery* ) arg;
    while(1){
        sleep(2);
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        for(int i = 0; i < MAX_PEERS; i++){
            
            // Critical section: fetch .is_empty
            pthread_mutex_lock(&peer_table->Node[i].lock_empty);
            bool empty = peer_table->Node[i].is_empty;
            pthread_mutex_unlock(&peer_table->Node[i].lock_empty); /*This variable MUST be protected by a mutex*/

            if(empty) continue;
            /*Elapsed checks whether the now time minus arrival time is greater than timeout*/
            double elapsed =
                (now.tv_sec - peer_table -> Node[i].last_hello.tv_sec) * 1e3 +
                (now.tv_nsec - peer_table -> Node[i].last_hello.tv_nsec) / 1e6;
            printf("sec_diff: %ld, nsec_diff: %ld, elapsed: %lf ms\n",
            (long)(now.tv_sec - peer_table->Node[i].last_hello.tv_sec),
            (now.tv_nsec - peer_table->Node[i].last_hello.tv_nsec),
                elapsed);

            if (elapsed > peer_table->timeout){
                printf("%lf",elapsed);
                printf("Client expired, node number %d has been removed\n",i);
                
                // Remove node
                pthread_mutex_lock(&peer_table -> Node[i].lock_empty);
                peer_table -> Node[i].is_empty = true;
                pthread_mutex_unlock(&peer_table -> Node[i].lock_empty);

            }
        }
    }
    return NULL;
}
