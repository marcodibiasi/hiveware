#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "utils.h"
#include "node.h"
#include "peer_disc.h"
#include "uv.h"
#include "client_info.h"

/*
 * Node lifecycle workflow
 *
 * CONFIGURATION
 *        |
 *        v
 * INITIALIZATION
 *   ├─ setup state
 *   ├─ init UDP/TCP
 *   └─ build peer table
 *        |
 *        v
 * RUNTIME
 *   ├─ UDP → hello / discovery / heartbeat
 *   └─ TCP → task execution / data exchange
 *        |
 *        v
 * TERMINATION
 *   ├─ stop condition
 *   ├─ join threads
 *   └─ close sockets
 */


int main(int arg, char* argv[]) {
    NodeConfig nconfig = default_nodeconfig(); 
    Node node = init_node(nconfig);

     start_node(&node);

    // -- TEST --
    // int count; 
    // uv_cpu_info_t* cpu_info = self_cpu_getinfo(&count);
    // print_cpu_info(cpu_info, count);

    pause();
    stop_node(&node);

    return EXIT_SUCCESS;
}
