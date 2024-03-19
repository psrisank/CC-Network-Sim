#ifndef __SWITCH_NODE_H__
#define __SWITCH_NODE_H__

#include "stdint.h"
#include "port.h"

// definitions for memory node
#define SW_NUM_TOP_PORTS	1
#define SW_NUM_BOT_PORTS	1
#define SW_QUEUE_SIZE		256

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