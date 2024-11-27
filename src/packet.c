#include "stdint.h"

#include <stdio.h>
#include "packet.h"


double calculate_packet_size(flag_t type, uint64_t addr_size, uint64_t data_size) {
    int bits = 0;
    if (type == INVALIDATE) {
        if (MULTICAST) {
            bits += 4; // type
            bits += 7; // src
            bits += 7; // dst
            bits += 128; // invalidates
            bits += addr_size*8;
        }
        else {
            bits += 4; // type
            bits += 7; // src
            bits += 7; // dst
            bits += addr_size*8;
        }
    }
    else if (type == TRANSFER) {
        bits += 4; // type
        bits += 7; // src
        bits += 7; // dst
        bits += addr_size*8; // address
        bits += data_size*8; // data
    }
    else if (type == STATECHANGE) {
        bits += 4;
        bits += 7;
        bits += 7;
        bits += addr_size*8;
    }
    double bytes = bits / 8;
    return bytes;
}