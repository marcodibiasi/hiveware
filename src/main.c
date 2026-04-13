#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "utils.h"
#include "node.h"
#include "peer_disc.h"


int main(int arg, char* argv[]) {
    NodeConfig nconfig = default_nodeconfig(); 
    Node node = init_node(nconfig);

    start_node(&node);
    pause();
    stop_node(&node);

    return EXIT_SUCCESS;
}
