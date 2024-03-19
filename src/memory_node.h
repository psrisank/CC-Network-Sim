#ifndef __MEMORY_NODE_H__
#define __MEMORY_NODE_H__

#include "stdint.h"
#include "packet.h"

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
	Packet top_ports_tx[MEM_NUM_TOP_PORTS][MEM_QUEUE_SIZE];
	uint32_t top_head_tx;
	uint32_t top_tail_tx;
	Packet top_ports_rx[MEM_NUM_TOP_PORTS][MEM_QUEUE_SIZE];
	uint32_t top_head_rx;
	uint32_t top_tail_rx;
}
MemoryNode;

// enum defining various cache states for MSI protocol
enum StatesMSI
{
	MODIFIED, SHARED, INVALID
};

// struct holding data inside a MemoryNode
typedef struct MemoryLine
{
	uint8_t address;
	uint32_t value;
	// status of all control nodes
	enum StatesMSI state;
}
MemoryLine;


#endif