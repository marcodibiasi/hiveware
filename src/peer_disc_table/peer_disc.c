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
    if(pthread_mutex_init(&pt->table_mutex, NULL) != 0)
        exit_error("pthread_mutex_init table_mutex");

    return pt;
}


void peer_discovery_destroy(PeerDiscovery* peer_table){
    if(!peer_table) return;
   
    pthread_mutex_destroy(&peer_table->table_mutex);
    free(peer_table);
}


int is_inside(PeerDiscovery* peer_table, uuid_t node_id){
    pthread_mutex_lock(&peer_table->table_mutex);
    
    for(int i = 0; i < peer_table->n_nodes; i++){
        if(memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0)
            return i;
    }

    pthread_mutex_unlock(&peer_table->table_mutex);
    return -1;
}


// TODO: pass and copy the ipv4 address
int peer_add(PeerDiscovery* peer_table, uuid_t node_id){
    if(!peer_table)
        exit_error("peer_table does not exist");

    int index = is_inside(peer_table,node_id);
    if(index != -1){
        pthread_mutex_lock(&peer_table->table_mutex);
        update_client_timer(&peer_table->Node[index]);
        pthread_mutex_unlock(&peer_table->table_mutex);
        return index;
    }
    
    // Add the new peer to the last slot available -> [n_nodes]
    pthread_mutex_lock(&peer_table->table_mutex);
    int i = peer_table->n_nodes;
    if(i >= MAX_PEERS) return -1;

    memcpy(peer_table->Node[i].node_id, node_id, sizeof(uuid_t));
    update_client_timer(&peer_table->Node[i]);

    peer_table->n_nodes++;
    pthread_mutex_unlock(&peer_table->table_mutex);

    return i; // Peer index inside the table
}


int peer_remove(PeerDiscovery* peer_table, uuid_t node_id){
    if(!peer_table)
        exit_error("peer_table does not exist");

    pthread_mutex_lock(&peer_table->table_mutex);
    for(int i = 0; i < peer_table->n_nodes; i++){

        // Found
        if(memcmp(peer_table->Node[i].node_id, node_id, sizeof(uuid_t)) == 0) {
            peer_remove_index(peer_table, i);    
            pthread_mutex_unlock(&peer_table->table_mutex);
            return 0; // Found 
        }
    }
    pthread_mutex_unlock(&peer_table->table_mutex);

    return -1; // node_id not found
}


// REQUIRES: table_mutex locked
void peer_remove_index(PeerDiscovery* peer_table, int index){
    /*  Switch the node with the last one and decrements the global variable n_nodes
        this ensures a packed array i.e. an array with contigous elements 
    */
    int last = peer_table->n_nodes - 1;
    if(index != last) 
        peer_table->Node[index] = peer_table->Node[last];

    peer_table->n_nodes--;
}


// REQUIRES: table_mutex locked
void update_client_timer(DiscNode* Node){
    clock_gettime(CLOCK_MONOTONIC, &Node->last_hello);
}


void* peer_daemon(void* arg){
    /*This function checks whether a node has reached the time out 
      if so, it removes the node from the peer_table

      Node is the calling client, that contains a peer table with all the nodes that run hiwa
    */

    Node* node = (Node*)arg;
    PeerDiscovery* peer_table = node->peer_table;

    while(atomic_load(&node->running)){
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        // Loop increments only if the current node is not expired, to not skip nodes (see remove logic)
        pthread_mutex_lock(&peer_table->table_mutex);
        for(int i = 0; i < peer_table->n_nodes; ){
            double elapsed =
                (now.tv_sec - peer_table->Node[i].last_hello.tv_sec) * 1e3 +
                (now.tv_nsec - peer_table->Node[i].last_hello.tv_nsec) / 1e6;
            
            printf("%lf\n", elapsed);
            peer_table->Node[i].elapsed = elapsed;

            if (elapsed > peer_table->timeout){
                printf("Peer expired: node number %d has been removed\n",i);
                peer_remove_index(peer_table, i);

                continue;
            }

            i++;
        }
        pthread_mutex_unlock(&peer_table->table_mutex);

        sleep(2);
    }
    return NULL;
}


void* print_daemon(void* arg){
    Node* node = (Node*)arg;

    while(atomic_load(&node->running)){
        // printf("\033[H\033[J");

        pthread_mutex_lock(&node->peer_table->table_mutex);
        print_peer_table(node->peer_table);
        pthread_mutex_unlock(&node->peer_table->table_mutex);

        sleep(1);
    }

    return NULL;
}


void print_peer_table(PeerDiscovery* pt){
    printf("\n\n");
    printf("+--------------------------------------+------------------+---------------------+\n");
    printf("| NODE_ID                              | ADDRESS          | LAST_HELLO          |\n");
    printf("+--------------------------------------+------------------+---------------------+\n");
    for(int i=0; i < pt->n_nodes; i++) {
        char uuid_str[37];
        char addr_str[32];
        // double last_hello = timespec_to_double(&pt.Node[i].last_hello);

        uuid_unparse(pt->Node[i].node_id, uuid_str);
        addr_to_string(&pt->Node[i].addr, addr_str);

        printf("| %-36s | %-16s | %-17.3f s |\n",
               uuid_str,
               addr_str,
               pt->Node[i].elapsed / 1e3);
    }

    printf("+--------------------------------------+------------------+---------------------+\n");
}
