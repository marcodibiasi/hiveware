#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "peer_disc.h"
#include <pthread.h>
#include "utils.h"

int main(int arg, char* argv[]) {
    NodeConfig config = default_nodeconfig();
    Node n = init_node(&config);
    
    /*ANDREA*/
    /*Istanzio il daemon che controlla continuamente se ci sono peers che non rispondono da vari secondi e creo il thread*/
    pthread_t dmn;
    if(pthread_create(&dmn, NULL, peer_daemon, (void*)n.peer_table) != 0)
		exit_error("pthread_create h_send");
        /*Non aggiungo pthread_join perché il daemon è un ciclo while infinito*/
    /*-----*/
    hello_handler(&n);

    printf("\nWork in progress...\n");
    return EXIT_SUCCESS;
}