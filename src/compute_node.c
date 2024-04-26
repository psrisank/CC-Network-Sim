#include "compute_node.h"

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

int read_action(ComputeNode node, uint32_t address) {
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

void write_action(ComputeNode* node, uint32_t address, uint32_t wdata) {
	int index = address % 4;
	node->cache[index].value = wdata;
	node->cache[index].address = address;
}

void cnode_process_packet(ComputeNode* node, Packet pkt) {
	if (pkt.flag == INVALIDATE) {
		node->cache[pkt.data.addr % 4].state = INVALID;
		node->cache[pkt.data.addr % 4].valid = 0;
	}
	else if (pkt.flag == NORMAL) {
		node->cache[pkt.data.addr % 4].value = pkt.data.data;
		node->cache[pkt.data.addr % 4].address = pkt.data.addr;
		node->cache[pkt.data.addr % 4].valid = 1;
		node->cache[pkt.data.addr % 4].state = SHARED;
	}
}