#ifndef __SWITCH_NODE_H__
#define __SWITCH_NODE_H__

#include "stdint.h"
#include "packet.h"

// definitions for memory node
#define SW_NUM_TOP_PORTS	1
#define SW_NUM_BOT_PORTS	1
#define SW_QUEUE_SIZE		256

// top-level struct for a switch node
typedef struct SwitchNode
{
	uint8_t id;
	uint32_t time;

	Packet top_ports_tx[SW_NUM_TOP_PORTS][SW_QUEUE_SIZE];
	uint32_t top_head_tx;
	uint32_t top_tail_tx;
	Packet top_ports_rx[SW_NUM_TOP_PORTS][SW_QUEUE_SIZE];
	uint32_t top_head_rx;
	uint32_t top_tail_rx;
	Packet bot_ports_tx[SW_NUM_BOT_PORTS][SW_QUEUE_SIZE];
	uint32_t bot_head_tx;
	uint32_t bot_tail_tx;
	Packet bot_ports_rx[SW_NUM_BOT_PORTS][SW_QUEUE_SIZE];
	uint32_t bot_head_rx;
	uint32_t bot_tail_rx;
}
SwitchNode;

#endif