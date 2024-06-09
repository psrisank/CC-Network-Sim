#include "compute_node.h"

long invalidation_msg = 0;
long control_node_write_message = 0;
long controlnode_request_to_memnode = 0;

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
	int index = (address >> 2) % 4;
	//printf("For address 0x%x, in node %d, cache state is: %s.\n", address, node.id, node.cache[index].valid ? "VALID": "INVALID");
	//control_message_compute_node_global_counter++;

	if (node.cache[index].valid == 1) {
		if (node.cache[index].address == address) { // cache hit
			return 0; // no need to retrieve data from mem (MODIFIED)
		}
		else {
			if (node.cache[index].dirty) { // cache miss with wb first
				return 2;
			}
			else { // cache miss w/o wb
				controlnode_request_to_memnode += 1;
				return 1;
			}
		}
	}
	else {
		controlnode_request_to_memnode += 1;
		return 1; // yea just replace
	}
}

void write_action(ComputeNode* node, uint32_t address, uint32_t wdata) {
	int index = (address >> 2) % 4;
	//control_message_compute_node_global_counter++;
    //printf("Told to write %x to address %x\n", wdata, address);
	node->cache[index].value = wdata;
	node->cache[index].address = address;
    node->cache[index].valid = 1;
	node->cache[index].state = SHARED;
	control_node_write_message += 1;
}

void cnode_process_packet(ComputeNode* node, Packet pkt, int* stall) {
	if (pkt.flag == INVALIDATE) {
		invalidation_msg++;
		//printf("Invalidated.\n");
		*stall = 0;
		if (pkt.data.addr == node->cache[(pkt.data.addr >> 2) % 4].address)
		{	
			//printf("Invalidating address %d in node: %d\n", pkt.data.addr, node->id);
			node->cache[(pkt.data.addr >> 2) % 4].state = INVALID;
			node->cache[(pkt.data.addr >> 2) % 4].valid = 0;
		}
	}
	else if (pkt.flag == NORMAL) {
		//printf("Received packet with data in node: %d\n", node->id);
		node->cache[(pkt.data.addr >> 2) % 4].value = pkt.data.data;
		node->cache[(pkt.data.addr >> 2) % 4].address = pkt.data.addr;
		node->cache[(pkt.data.addr >> 2) % 4].valid = 1;
		node->cache[(pkt.data.addr >> 2) % 4].state = SHARED;
		//printf("Address %d is now valid in cache.\n", pkt.data.addr);
		*stall = 0;
	}
}

void print_cacheline(int index, ComputeNodeMemoryLine line) {
    char* state = line.state == MODIFIED ? "MODIFIED" : line.state == INVALID ? "INVALID" : "SHARED";
    printf("index: %d | address: %08x | value: %08x | valid: %d | state: %s \n", index, line.address, line.value, line.valid, state);
}

void print_cache(ComputeNode* node) {
    for (int i = 0; i < CACHE_LINES; i++) {
        print_cacheline(i, node->cache[i]);
    }
}

void get_compute_control_count()
{
	printf("\n\nInvalidation Messages from switch: %ld\n", invalidation_msg);
	printf("Write messages to memory nodes: %ld\n", control_node_write_message);
	printf("Data request messages to memory nodes: %ld\n", controlnode_request_to_memnode - 128);
}