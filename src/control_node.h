#ifndef __CONTROL_NODE_H__
#define __CONTROL_NODE_H__

#include "stdint.h"

// definitions for memory node
#define NUM_PORTS 1

// top-level struct for a memory node
typedef struct ControlNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
}
ControlNode;

// struct holding data inside a ControlNode
typedef struct ControlNodeMemoryLine
{
	uint8_t address;
	uint32_t value;
	// status of all control nodes
}
ControlNodeMemoryLine;

#endif