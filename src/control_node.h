#ifndef __CONTROL_NODE_H__
#define __CONTROL_NODE_H__

#include "stdint.h"
#include "packet.h"

// definitions for memory node
#define NUM_PORTS			1
#define CTRL_NUM_BOT_PORTS	1
#define CTRL_QUEUE_SIZE		256

// top-level struct for a memory node
typedef struct ControlNode
{
	uint8_t id;
	uint32_t time;
	// array of ports (queues) -> needs to contain a Packet struct type
	// array of MemoryLines
	Packet bot_ports_tx[CTRL_NUM_BOT_PORTS][CTRL_QUEUE_SIZE];
	uint32_t bot_head_tx;
	uint32_t bot_tail_tx;
	Packet bot_ports_rx[CTRL_NUM_BOT_PORTS][CTRL_QUEUE_SIZE];
	uint32_t bot_head_rx;
	uint32_t bot_tail_rx;
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