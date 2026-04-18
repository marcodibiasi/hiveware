#ifndef UDP_H
#define UDP_H

#include <stdlib.h>

typedef struct Node Node; // forward declaration

void init_h_socket(Node* n);
void* send_hello(void* arg);
void* recv_hello(void* arg);

#endif 
