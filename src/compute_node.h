#ifndef __COMPUTE_NODE_H__
#define __COMPUTE_NODE_H__

#include "stdint.h"
#include "port.h"

// definitions for memory node
#define CMP_NUM_BOT_PORTS	1	// should not ever be changed
#define CMP_QUEUE_SIZE		256	// in packets
#define CACHE_LINES			4


// struct holding data inside a ComputeNode
typedef struct ComputeNodeMemoryLine
{
	uint8_t address;
	uint32_t value;
	// status of all compute nodes
	uint8_t valid;
	uint8_t dirty;
}
ComputeNodeMemoryLine;

// top-level struct for a memory node
typedef struct ComputeNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
	Port bot_ports[CMP_NUM_BOT_PORTS];
	ComputeNodeMemoryLine cache[CACHE_LINES];
}
ComputeNode;


#endif