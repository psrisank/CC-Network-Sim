#ifndef __MEMORY_NODE_H__
#define __MEMORY_NODE_H__

#include "stdint.h"

// definitions for memory node
#define NUM_PORTS 1

// top-level struct for a memory node
typedef struct MemoryNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
}
MemoryNode;

// struct holding data inside a MemoryNode
typedef struct MemoryLine
{
	uint8_t address;
	uint32_t value;
	// status of all control nodes
}
MemoryLine;

// enum defining various cache states for MSI protocol
const enum StatesMSI
{
	MODIFIED, SHARED, INVALID
};

#endif