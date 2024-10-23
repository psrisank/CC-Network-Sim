#include "stdint.h"

#include <stdio.h>
#include "packet.h"


double calculate_packet_size(flag_t type) {
    int bits = 0;
    if (type == INVALIDATE) {
        if (MULTICAST) {
            bits += 4; // type
            bits += 7; // src
            bits += 7; // dst
            bits += 128; // invalidates
            bits += ADDR_SIZE*8;
        }
        else {
            bits += 4; // type
            bits += 7; // src
            bits += 7; // dst
            bits += ADDR_SIZE*8;
        }
    }
    else if (type == TRANSFER) {
        bits += 4; // type
        bits += 7; // src
        bits += 7; // dst
        bits += ADDR_SIZE*8; // address
        bits += OBJ_SIZE*8; // data
    }
    else if (type == STATECHANGE) {
        bits += 4;
        bits += 7;
        bits += 7;
        bits += 64;
        bits += 3;
    }
    double bytes = bits / 8;
    return bytes;
}