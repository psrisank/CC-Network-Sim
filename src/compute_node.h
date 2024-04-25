#ifndef __COMPUTE_NODE_H__
#define __COMPUTE_NODE_H__

#include "stdint.h"
#include "port.h"
#include "math.h"
#include "packet.h"

// definitions for memory node
#define CMP_NUM_BOT_PORTS	1	// should not ever be changed
#define CMP_QUEUE_SIZE		256	// in packets
#define BLOCK_SIZE			1
#define WORD_SIZE			32
#define CACHE_LINES			4

typedef enum {
	INVALID,
	SHARED,
	MODIFIED,
} state_t;

// struct holding data inside a ComputeNode
typedef struct ComputeNodeMemoryLine
{
	uint32_t address;
	uint32_t value;
	// status of all compute nodes
	uint8_t valid;
	uint8_t dirty;
	state_t state;
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
	// last used cache line (for LRU policy)
	int last_used;
}
ComputeNode;

void updateNodeState(ComputeNode* node) {
	for (int i = 0; i < CACHE_LINES; i++) {
		if (node->cache[i].valid && node->cache[i].dirty) {
			node->cache[i].state = MODIFIED;
		}
		else if (node->cache[i].valid && !node->cache[i].dirty) {
			node->cache[i].state = SHARED;
		}
		else {
			node->cache[i].state = INVALID;
		}
	}
}

int requestData(ComputeNode node, uint32_t address) {
	int index = address % 4;
	if (node.cache[index].valid == 1) {
		if (node.cache[index].address == address) { // cache hit
			return 0; // no need to retrieve data from mem (MODIFIED)
		}
		else {
			if (node.cache[index].dirty) { // cache miss with wb first
				return 2;
			}
			else { // cache miss w/o wb
				return 1;
			}
		}
	}
	else {
		return 1; // yea just replace
	}
}

void cnode_process_packet(ComputeNode node, Packet pkt) {

}


#endif