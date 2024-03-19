#ifndef __MEMORY_NODE_H__
#define __MEMORY_NODE_H__

#include "stdint.h"
#include "port.h"

// definitions for memory node
#define NUM_PORTS			1
#define MEM_NUM_TOP_PORTS	1
#define MEM_QUEUE_SIZE		256

// top-level struct for a memory node
typedef struct MemoryNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
	Port top_ports[MEM_NUM_TOP_PORTS];
}
MemoryNode;

// enum defining various cache states for MSI protocol
typedef enum
{
	MODIFIED, SHARED, INVALID
}
StatesMSI_t;

// struct holding data inside a MemoryNode
typedef struct MemoryLine
{
	uint8_t address;
	uint32_t value;
	// status of all compute nodes
	StatesMSI_t state;
}
MemoryLine;


#endif