#include "compute_node.h"
#include "memory_node.h"

long invalidation_msg = 0;
long control_node_write_requests = 0;
long control_node_data_requests = 0;
long control_node_data_returns = 0;

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

int check_state(ComputeNode node, uint32_t address) {
	int index = (address >> 2) % 4;
	if (node.cache[index].address == address) {
		if (!node.cache[index].valid) {
			return 1; // addr matches, invalid
		}
		else {
			if (node.cache[index].dirty) {
				return 2; // addr matches, modified
			}
			else {
				return 3; // addr matches, shared
			}
		}
	}
	else {

		if (node.cache[index].valid) {
			// printf("Cache conflict!!!! for address 0x%lx\n", (long unsigned int) address);
			// printf("Conflicts with address 0x%lx\n", (long unsigned int) node.cache[index].address);
			if (node.cache[index].dirty) {
				return 4; // addr doesn't match, modified
			}
			else {
				// printf("Conflicts with address 0x%lx\n", (long unsigned int) node.cache[index].address);
				// printf("Requesting new data.\n");
				return 5; // addr doesn't match, shared
			}
		}
		else {
			return 6; // addr doesn't match, invalid
		}
	}
}

void log_cdatareq() {
	control_node_data_requests++;
}

void log_cwritereq() {
	control_node_write_requests++;
}

void log_cwritedata() {
	control_node_data_returns++;
}

void unlog_cdatareq() {
	control_node_data_requests--;
}


void write_action(ComputeNode* node, uint32_t address, uint32_t wdata) {
	int index = (address >> 2) % 4;
	//control_message_compute_node_global_counter++;
    //printf("Told to write %x to address %x\n", wdata, address);
	node->cache[index].value = wdata;
	node->cache[index].address = address;
    node->cache[index].valid = 1;
	node->cache[index].dirty = 1;
	node->cache[index].state = MODIFIED;
}

Packet cnode_process_packet(ComputeNode* node, Packet pkt, int* stall) { // packets external to the instruction queue
	Packet ret_pkt;
	ret_pkt.flag = ERROR;
	if (pkt.flag == RESEND) {
		ret_pkt.flag = RESEND;
	}
	else if (pkt.flag == INVALIDATE) {
		invalidation_msg++;
		//printf("Invalidated.\n");
		*stall = 0;
		// if (pkt.data.addr == node->cache[(pkt.data.addr >> 2) % 4].address)
		// {	
			//printf("Invalidating address %d in node: %d\n", pkt.data.addr, node->id);
			node->cache[(pkt.data.addr >> 2) % 4].state = INVALID;
			node->cache[(pkt.data.addr >> 2) % 4].valid = 0;
		// }
	}
	else if (pkt.flag == NORMAL) {
		// printf("Received packet with data in node: %d\n", node->id);
		node->cache[(pkt.data.addr >> 2) % 4].value = pkt.data.data;
		node->cache[(pkt.data.addr >> 2) % 4].address = pkt.data.addr;
		node->cache[(pkt.data.addr >> 2) % 4].valid = 1;
		node->cache[(pkt.data.addr >> 2) % 4].state = SHARED;
		node->cache[(pkt.data.addr >> 2) % 4].dirty = 0;
		// printf("Address 0x%lx is now valid in cache.\n", pkt.data.addr);
		*stall = 0;
	}
	else if (pkt.flag == READ) { // essentially a read from the memory
		// printf("Node %d returning data.\n", node->id);
		ret_pkt.id = 0;
		ret_pkt.time = 0;
		ret_pkt.flag = WR_DATA;
		ret_pkt.src = node->id;
		ret_pkt.dst = pkt.data.addr;
		ret_pkt.data.addr = pkt.data.addr;
		ret_pkt.data.data = node->cache[(pkt.data.addr >> 2) % 4].value;
		node->cache[(pkt.data.addr >> 2) % 4].state = SHARED;
		node->cache[(pkt.data.addr >> 2) % 4].dirty = 0;
		log_cwritedata();
	}
	return ret_pkt;

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
	printf("Write requests to memory nodes: %ld\n", control_node_write_requests);
	printf("Read requests to memory nodes: %ld\n", control_node_data_requests);
	printf("Memory to compute node data requests: %ld\n", get_memory_to_compute_requests());
	printf("Data return/writebacks to memory nodes: %ld\n", control_node_data_returns);
}