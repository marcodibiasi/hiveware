#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include "peer_disc.h"
#include "utils.h"
#include "node.h"


PeerDiscovery* peer_discovery_init(uint32_t timeout){
    PeerDiscovery* pt = calloc(1, sizeof(PeerDiscovery));
    if(!pt) exit_error("Calloc Failed");
    
    pt->n_nodes = 0;
    pt->timeout = timeout;
    if(pthread_mutex_init(&pt->table_mutex, NULL) == 0)
        exit_error("pthread_mutex_init table_mutex");

    // Initialize each mutex
    for(int i = 0; i<MAX_PEERS;i++){
        pt->Node[i].is_empty = true;
        if(pthread_mutex_init(&pt->Node[i].mutex, NULL) != 0){
            for(int j = 0; j < i; j++) {
                pthread_mutex_destroy(&pt->Node[j].mutex);
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
        pthread_mutex_destroy(&peer_table -> Node[i].mutex);
    }
    
    free(peer_table);
}


int is_inside(PeerDiscovery* peer_table, uuid_t node_id){
    pthread_mutex_lock(&peer_table->table_mutex);
    
    for(int i = 0; i < peer_table->n_nodes; i++){
        bool empty = peer_table->Node[i].is_empty;
        bool match = memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0;
        
        if(!empty && match) return i;
    }

    pthread_mutex_unlock(&peer_table->table_mutex);
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
    
    // ADD THE NEW PEER TO THE LAST SLOT AVAILABLE -> [n_nodes]
    pthread_mutex_lock(&peer_table->table_mutex);
    int i = peer_table->n_nodes;
    if(i >= MAX_PEERS) return -1;

    peer_table->Node[i].is_empty = false;
    memcpy(peer_table->Node[i].node_id, node_id, sizeof(uuid_t));
    update_client_timer(&peer_table->Node[i]);

    peer_table->n_nodes++;
    pthread_mutex_unlock(&peer_table->table_mutex);

    return -1; /*No peers available*/
}


int peer_remove(PeerDiscovery* peer_table, uuid_t node_id){
    if(!peer_table)
        exit_error("peer_table does not exist");

    pthread_mutex_lock(&peer_table->table_mutex);
    for(int i = 0; i < peer_table->n_nodes; i++){
        /*If we find an empty node, we just add the uuid and change the boolean flag to True*/
        if(memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0) {
            peer_table->Node[i].is_empty = true;        
            
            // Move the last element to the slot removed to keep the queue packed
            peer_table->n_nodes--;
            peer_table->Node[i] = peer_table->Node[peer_table->n_nodes];
            pthread_mutex_unlock(&peer_table->table_mutex);

            return 0; // Found 
        }
    }
    pthread_mutex_unlock(&peer_table->table_mutex);

    return -1; // node_id not found
}


void update_client_timer(DiscNode* Node){
    clock_gettime(CLOCK_MONOTONIC, &Node->last_hello);
}


void* peer_daemon(void* arg){
    /*This function checks whether a node has reached the time out 
      if so, it removes the node from the peer_table
    */
    Node* node = (Node*)arg;
    PeerDiscovery* peer_table = node->peer_table;
    while(atomic_load(&node->running)){
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        for(int i = 0; i < MAX_PEERS; i++){
            
            // Critical section: fetch .is_empty
            pthread_mutex_lock(&peer_table->Node[i].mutex);
            bool empty = peer_table->Node[i].is_empty;
            pthread_mutex_unlock(&peer_table->Node[i].mutex); /*This variable MUST be protected by a mutex*/

            if(empty) continue;
            /*Elapsed checks whether the now time minus arrival time is greater than timeout*/
            double elapsed =
                (now.tv_sec - peer_table -> Node[i].last_hello.tv_sec) * 1e3 +
                (now.tv_nsec - peer_table -> Node[i].last_hello.tv_nsec) / 1e6;

            peer_table->Node[i].elapsed = elapsed;

            /* printf("sec_diff: %ld, nsec_diff: %ld, elapsed: %lf ms\n",
            (long)(now.tv_sec - peer_table->Node[i].last_hello.tv_sec),
            (now.tv_nsec - peer_table->Node[i].last_hello.tv_nsec),
                elapsed); */


            if (elapsed > peer_table->timeout){
                printf("%lf",elapsed);
                printf("Client expired, node number %d has been removed\n",i);
                
                // Remove node
                pthread_mutex_lock(&peer_table -> Node[i].mutex);
                peer_table -> Node[i].is_empty = true;
                pthread_mutex_unlock(&peer_table -> Node[i].mutex);

            }
        }
        sleep(2);
    }
    return NULL;
}


void* print_daemon(void* arg){
    Node* node = (Node*)arg;

    while(atomic_load(&node->running)){
        printf("\033[H\033[J");
        print_peer_table(node->peer_table);
        sleep(1);
    }

    return NULL;
}


void print_peer_table(PeerDiscovery pt){
    printf("\n\n");
    printf("+--------------------------------------+------------------+---------------------+\n");
    printf("| NODE_ID                              | ADDRESS          | LAST_HELLO          |\n");
    printf("+--------------------------------------+------------------+---------------------+\n");
    for(int i=0; i < pt.n_nodes; i++) {
        if(pt.Node[i].is_empty)
            continue;

        char uuid_str[37];
        char addr_str[32];
        // double last_hello = timespec_to_double(&pt.Node[i].last_hello);

        uuid_unparse(pt.Node[i].node_id, uuid_str);
        addr_to_string(&pt.Node[i].addr, addr_str);

        printf("| %-36s | %-16s | %-17.3f s |\n",
               uuid_str,
               addr_str,
               pt.Node[i].elapsed / 1e3);
    }

    printf("+--------------------------------------+------------------+---------------------+\n");
}
