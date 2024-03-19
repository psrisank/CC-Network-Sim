#ifndef __PACKET_H__
#define __PACKET_H__

#include "stdint.h"

// struct holding data inside a Packet
typedef struct DataNode
{
	// fields
	uint8_t type;
	uint32_t addr;
	uint32_t data;
}
DataNode;

// top-level struct for a packet
typedef struct Packet
{
	uint8_t id;
	uint32_t time;
	
	uint8_t src;
	uint8_t dst;
	DataNode data;
}
Packet;

#endif