#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>
#include <uuid/uuid.h>
#include "node.h"
#include "udp.h"
#include "tcp.h"
#include "utils.h"
#include "peer_disc.h"


NodeConfig default_nodeconfig(void) {
    NodeConfig c = {0}; 
    strcpy(c.h_multicast, "239.255.0.1");
    c.h_port = 50000;
    c.c_port = 50001;
    c.heartbeat_ms = 2000;
    c.timeout_ms = 8000;
    
    printf("default nodeconfig created: \nh_port: %d" 
            "\nc_port: %d \nh_multicast: %s"
            "\nheartbeat_ms = %d \ntimeout_ms = %d\n",
            c.h_port, c.c_port, c.h_multicast, c.heartbeat_ms, c.timeout_ms);
    return c;
}


Node init_node(NodeConfig config) {    
    Node node = {0};
    node.config = config; 
	load_or_create_uuid(node.id);
    atomic_init(&node.running, false);
    node.peer_table = peer_discovery_init(node.config.timeout_ms);

    cpu_capacity(&node.n_cores, &node.avg_mhz);
    node.last_cpu_snapshot = cpu_times_snapshot();

    node.job_registry = job_registry_init();
    node.task_executor = NULL; // impostare esplicitamente prima di start_node se si vuole eseguire task ricevuti

	init_h_socket(&node);
    init_c_socket(&node);

    print_uuid(node.id);
    printf("capacity: %d cores @ %d MHz (avg)\n", node.n_cores, node.avg_mhz);
    return node;
}


void start_node(Node* node){
    atomic_store(&node->running, true);

    // DAEMONS
    if(pthread_create(&node->nthreads.peer_table_daemon, NULL, peer_daemon, (void*)node) != 0)
		exit_error("pthread_create peer_table_daemon");
    if(pthread_create(&node->nthreads.print_table_daemon, NULL, print_daemon, (void*)node) != 0)
		exit_error("pthread_create print_table_daemon");


    // HELLO PROCESSES
 	if(pthread_create(&node->nthreads.h_send, NULL, send_hello, (void*)node) != 0)
		exit_error("pthread_create h_send");	
	if(pthread_create(&node->nthreads.h_recv, NULL, recv_hello, (void*)node) != 0)
		exit_error("pthread_create h_recv");

    // TCP TASK LAYER
    if(pthread_create(&node->nthreads.tcp_accept, NULL, tcp_accept_daemon, (void*)node) != 0)
        exit_error("pthread_create tcp_accept");
}


void stop_node(Node* node){
    atomic_store(&node->running, false);
    shutdown(node->h_socket, SHUT_RDWR);
    shutdown(node->c_socket, SHUT_RDWR); // sblocca accept() nel tcp_accept_daemon

    // DAEMONS
    if(pthread_join(node->nthreads.peer_table_daemon, NULL) != 0)
        exit_error("pthread_join peer_table_daemon");
    if(pthread_join(node->nthreads.print_table_daemon, NULL) != 0)
        exit_error("pthread_join print_table_daemon");


    // HELLO PROCESSES
	if(pthread_join(node->nthreads.h_send, NULL) != 0)
		exit_error("pthread_join h_send");
	if(pthread_join(node->nthreads.h_recv, NULL) != 0)
		exit_error("pthread_join h_recv");

    // TCP TASK LAYER
    if(pthread_join(node->nthreads.tcp_accept, NULL) != 0)
        exit_error("pthread_join tcp_accept");
    // NOTA: le connessioni gia' accettate (tcp_conn_handler, thread detached)
    // non vengono joinate qui: si chiudono da sole al prossimo controllo di
    // node->running o alla disconnessione del peer.

    close(node->h_socket);
    close(node->c_socket);
    job_registry_destroy(node->job_registry); // non distrugge i Job, solo il registro (vedi job_registry.h)
}


void load_or_create_uuid(uuid_t id){
    FILE *f = fopen(".hiwaid", "r");

    if(f){
        char buffer[37];
        if(fgets(buffer, sizeof(buffer), f)){
            uuid_parse(buffer, id);
        } else {
            uuid_generate_random(id);
        }
        fclose(f);
    } else {
        uuid_generate_random(id);
        f = fopen(".hiwaid", "w");
        if(f){
            char buffer[37];
            uuid_unparse(id, buffer);
            fprintf(f, "%s\n", buffer);
            fclose(f);
        }
    }
}
