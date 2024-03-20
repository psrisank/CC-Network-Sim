#ifndef __SWITCH_NODE_H__
#define __SWITCH_NODE_H__

#include "stdint.h"
#include "port.h"
#include "main.h"

// definitions for memory node
#define SW_NUM_TOP_PORTS	NUM_COMPUTE_NODES
#define SW_NUM_BOT_PORTS	NUM_MEMORY_NODES
#define SW_QUEUE_SIZE		256	// in packets

// top-level struct for a switch node
typedef struct SwitchNode
{
	uint8_t id;
	uint32_t time;

	Port top_ports[SW_NUM_TOP_PORTS];
	Port bot_ports[SW_NUM_TOP_PORTS];
}
SwitchNode;

#endif