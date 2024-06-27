#include "compute_node.h"
#include "memory_node.h"

long invalidation_msg = 0;
long control_node_write_requests = 0;
long control_node_data_requests = 0;
long control_node_data_returns_to_cnodes = 0;
long control_node_data_returns = 0;
long state_change_ctrls = 0;

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

int check_state(ComputeNode* node, uint64_t address, int* recheck) {
	int index; // = (address >> 2) % 4;
	int addr_present = 0;
	for (int i = 0; i < CACHE_LINES; i++) {
		if (node->cache[i].address == address && node->cache[i].state != INVALID) {
			addr_present = 1;
			index = i;
			break;
		}
	}

	if (!addr_present) {
		if (*recheck != 0) {
			node->last_used = node->last_used - 1;
			if (node->last_used < 0) {
				node->last_used = CACHE_LINES - 1;
			}
		}
		index = node->last_used;
		// printf("address wasn't present, replacing index %d.\n", index);
		node->last_used = node->last_used + 1;
		if (node->last_used == CACHE_LINES) {
			node->last_used = 0;
		}
	}

	state_t cacheState = node->cache[index].state;
	node->idx_to_modify = index;
	if (node->cache[index].address == address) { // no need to evict
		if (cacheState == MODIFIED) {
			return 1;
		}
		else if (cacheState == OWNED) {
			return 2;
		}
		else if (cacheState == EXCLUSIVE) {
			return 3;
		}
		else if (cacheState == SHARED) {
			return 4;
		}
		else if (cacheState == INVALID) {
			return 5;
		}
	}
	else { // eviction required 
		if (cacheState == MODIFIED) {
			return 6;
		}
		else if (cacheState == OWNED) {
			return 7;
		}
		else if (cacheState == EXCLUSIVE) {
			return 8;
		}
		else if (cacheState == SHARED) {
			return 9;
		}
		else if (cacheState == INVALID) {
			return 10;
		}
	}
	return -1;
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


void write_action(ComputeNode* node, uint64_t address, uint32_t wdata) {
	// int index = (address >> 2) % 4;
	//control_message_compute_node_global_counter++;
    //printf("Told to write %x to address %x\n", wdata, address);
	// printf("Now modifying index %d.\n", node->idx_to_modify);
	node->cache[node->idx_to_modify].value = wdata;
	node->cache[node->idx_to_modify].address = address;
    node->cache[node->idx_to_modify].valid = 1;
	node->cache[node->idx_to_modify].dirty = 1;
	node->cache[node->idx_to_modify].state = MODIFIED;
}

Packet cnode_process_packet(ComputeNode* node, Packet pkt, int* stall, FILE* log_file, int global_time) { // packets external to the instruction queue
	Packet ret_pkt;
	ret_pkt.flag = ERROR;

	if (pkt.flag == INVALIDATE) {
		// printf("Node %d received invalidation. Now in invalid.\n", pkt.dst);
		invalidation_msg++;
		*stall = 0;
		int i;
		for (i = 0; i < CACHE_LINES; i++) {
			if (node->cache[i].address == pkt.data.addr) {
				// printf("Node %d received invalidation.\n", node->id);
				break;
			}
		}
		node->cache[i].state = INVALID;
		node->cache[i].valid = 0;
	}
	else if (pkt.flag == RESPONSE) {
		// printf("Node %d received data from a memory node. Now in exclusive for index %d.\n", node->id, node->idx_to_modify);
		// printf("Now modifying index %d for a read.\n", node->idx_to_modify);
		node->cache[node->idx_to_modify].value = pkt.data.data;
		node->cache[node->idx_to_modify].address = pkt.data.addr;
		node->cache[node->idx_to_modify].valid = 1;
		node->cache[node->idx_to_modify].state = EXCLUSIVE;
		node->cache[node->idx_to_modify].dirty = 0;
		// if (pkt.data.addr == 0) {
		// 	printf("Received data for address 0.\n");
		// 	printf("Has data 0x%x.\n", pkt.data.data);
		// }
		*stall = 0;
		state_change_ctrls++;
		fprintf(log_file, "Node %d received response for address 0x%lx at time %d\n\n\n", node->id, pkt.data.addr, global_time);
	}
	else if (pkt.flag == TRANSFER) { // essentially a read from the memory
		// printf("Node %d returning data.\n", node->id);
		// TODO: send the data to the source node
		int idxWithData;
		for (int i = 0; i < CACHE_LINES; i++) {
			if (node->cache[i].address == pkt.data.addr) {
				idxWithData = i;
				break;
			}
		}
		ret_pkt.id = 0;
		ret_pkt.time = 0;
		ret_pkt.flag = WR_DATA;
		ret_pkt.src = node->id;
		ret_pkt.dst = pkt.src;
		ret_pkt.data.addr = pkt.data.addr;
		ret_pkt.data.data = node->cache[idxWithData].value;
		// TODO: state change depending on the current state
		if (pkt.data.data == 0) {
			// printf("Node %d told to transfer to node %d. Now in owned.\n", node->id, pkt.src);
			if (node->cache[idxWithData].state != EXCLUSIVE) {
				state_change_ctrls++;
			}
			node->cache[idxWithData].state = OWNED;
		}
		else if (pkt.data.data == 1) {
			// printf("Node %d told to transfer to node %d. Now in shared.\n", node->id, pkt.src);
			if (node->cache[idxWithData].state != SHARED) {
				state_change_ctrls++;
			}
			node->cache[idxWithData].state = SHARED;
			state_change_ctrls++;
		}
		control_node_data_returns_to_cnodes++;
		// log_cwritedata();
	}
	else if (pkt.flag == WR_DATA) {
		// printf("Node %d received data from node %d. Now in shared.\n", node->id, pkt.src);;
		node->cache[node->idx_to_modify].valid = 1;
		node->cache[node->idx_to_modify].state = SHARED;
		state_change_ctrls++;
		node->cache[node->idx_to_modify].address = pkt.data.addr;
		node->cache[node->idx_to_modify].value = pkt.data.data;
		fprintf(log_file, "Node %d received response for address 0x%lx at time %d\n\n\n", node->id, pkt.data.addr, global_time);
	}
	return ret_pkt;

}

// void print_cacheline(int index, ComputeNodeMemoryLine line) {
//     char* state = line.state == MODIFIED ? "MODIFIED" : line.state == INVALID ? "INVALID" : "SHARED";
//     printf("index: %d | address: %08lx | value: %08x | valid: %d | state: %s \n", index, line.address, line.value, line.valid, state);
// }

// void print_cache(ComputeNode* node) {
//     for (int i = 0; i < CACHE_LINES; i++) {
//         print_cacheline(i, node->cache[i]);
//     }
// }

void get_statistics()
{
	printf("\n\nInvalidation Messages from switch: %ld\n", invalidation_msg);
	printf("Invalidations from memory to switch: %ld\n", get_memory_to_switch_invalidations());
	printf("Read requests to memory nodes: %ld\n", control_node_data_requests);
	printf("Write requests to memory nodes: %ld\n", control_node_write_requests);
	printf("Transfer requests from memory nodes: %ld\n", transfer_requests());
	printf("State change requests from memory node: %ld\n", state_change_ctrls);
	printf("Compute node to compute node data transfers: %ld\n", control_node_data_returns_to_cnodes);
	printf("Compute node to memory node data transfers: %ld\n", control_node_data_returns);
	printf("Memory node to compute node data transfer: %ld\n", get_memory_control_count());


}