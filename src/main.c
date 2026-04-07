#include <stdio.h>
#include <stdlib.h>
#include "node.h"

int main(int arg, char* argv[]) {
    NodeConfig config = default_nodeconfig();
    init_node(&config);

    printf("\nWork in progress...\n");
    return EXIT_SUCCESS;
}
