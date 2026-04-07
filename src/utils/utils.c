#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

void exit_error(char* msg){
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

bool mac_compare(uint8_t* mac_address_1, uint8_t* mac_address_2){
    /*just a comparison between mac_1 and mac_2*/
    for(int i = 0; i < 6; i++){
        if(mac_address_1[i] != mac_address_2[i]){
            return false;
        }
    }
    return true;
}
