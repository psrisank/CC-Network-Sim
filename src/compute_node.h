#ifndef __COMPUTE_NODE_H__
#define __COMPUTE_NODE_H__

#include "stdint.h"
#include "port.h"

// definitions for memory node
#define NUM_PORTS			1
#define CMP_NUM_BOT_PORTS	1
#define CMP_QUEUE_SIZE		256

// top-level struct for a memory node
typedef struct ComputeNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
	Port bot_ports[CMP_NUM_BOT_PORTS];
}
ComputeNode;

// struct holding data inside a ComputeNode
typedef struct ComputeNodeMemoryLine
{
	uint8_t address;
	uint32_t value;
	// status of all compute nodes
}
ComputeNodeMemoryLine;

#endif