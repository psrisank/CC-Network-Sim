#ifndef __PACKET_H__
#define __PACKET_H__

#include "stdint.h"

#define MAX_PKT_SIZE 8 // RDMA: 72 bytes | EDC: 8 bytes
#define MULTICAST 0 // Whether to multicast or not
#define OBJ_SIZE 4 // Bytes
#define ADDR_SIZE 8 // Bytes

// enum defining various cache states for MSI protocol
// typedef enum
// {
// 	NORMAL, READ, WR_DATA, ERROR, INVALIDATE, WR_SIGNAL, INST_WRITE, INST_READ, RESEND
// }
// flag_t;

typedef enum {
	RESPONSE, READ_REQUEST, TRANSFER, STATECHANGE, WR_REQUEST, WR_DATA, INST_WRITE, INST_READ, ERROR, INVALIDATE
} flag_t;

// struct holding data inside a Packet
typedef struct DataNode
{
	// fields
	uint64_t addr;
	uint32_t data;
}
DataNode;

// top-level struct for a packet
typedef struct Packet
{
	uint32_t id;
	uint32_t time;
	flag_t flag;
	
	uint8_t src;
	uint8_t dst;
	DataNode data;
	// uint8_t invalidates[128];
	uint8_t* invalidates;
}
Packet;

double calculate_packet_size(flag_t type);

#endif