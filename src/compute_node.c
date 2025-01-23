#include "compute_node.h"
#include "memory_node.h"
// #define CEIL(x) ((x) == (int)(x) ? (x) : (x) > 0 ? (int)((x) + 1) : (int)(x))
#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))
#define CEILING_NEG(X) (int)(X)
#define CEIL(X) ( ((X) > 0) ? CEILING_POS(X) : CEILING_NEG(X) )

long invalidation_msg = 0;
double invalidation_b = 0;

long control_node_write_requests = 0;
double control_node_write_requests_b = 0;

long control_node_data_requests = 0;
double control_node_data_requests_b = 0;

long control_node_data_returns_to_cnodes = 0;
double compute_to_compute_b = 0;

long control_node_data_returns = 0;
double compute_to_memory_b = 0;

long state_change_ctrls = 0;
double state_change_ctrl_bits = 0;

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

int check_state(ComputeNode* node, uint64_t address, int* recheck, int global_id, int global_time, Packet pkt) {
	int index; // = (address >> 2) % 4;
	int addr_present = 0;
	for (int i = 0; i < CACHE_LINES; i++) {
		if (node->cache[i].address == address && node->cache[i].state != INVALID) {
			addr_present = 1;
			index = i;
			// printf("Address present in cache of %d at index %d\n", node->id, i);
			break;
		}
	}

	if (!addr_present) {
		// printf("Address %lx not present in cache of %d. Recheck: %d. Index will now be: %u\n", address, node->id, *recheck, node->last_used);
		if (*recheck != 0) {
			node->last_used = node->last_used - 1;
			if (node->last_used < 0) {
				node->last_used = CACHE_LINES - 1;
			}
		}
		index = node->last_used;
		// Tell memory node that this node will be replaced
		if (node->cache[node->last_used].state == MODIFIED || node->cache[node->last_used].state == OWNED || node->cache[node->last_used].state == EXCLUSIVE || node->cache[node->last_used].state == SHARED) {
			// printf("Eviction!\n");
			Packet eviction_pkt = (Packet) {global_id, global_time, EVICTION, node->id, node->cache[node->last_used].address / MEM_NUM_LINES + 129, (DataNode) {node->cache[node->last_used].address, node->cache[node->last_used].value}, NULL, 0, 0}; // Writes data to the address
			push_packet(&(node->bot_ports[0]), TX, eviction_pkt);
			node->cache[node->last_used].state = INVALID;
			if (node->cache[node->last_used].state != EXCLUSIVE && node->cache[node->last_used].state != SHARED) { // Only count the packet if it actually necessitated a write
				control_node_data_returns++;
				if ((HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8) < 64*8) {
					compute_to_memory_b += 64*8;
				}
				else {
					compute_to_memory_b += HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8;
				}
			}
		}
		
		// printf("address wasn't present, replacing index %d in node %d.\n", index, node->id);
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

void log_cdatareq(Packet pkt) {
	control_node_data_requests++;
	// control_node_data_requests_b += HEADER_SIZE + pkt.address_size*8;
	if ((HEADER_SIZE + pkt.address_size*8) <= 64*8) {
		control_node_data_requests_b += 64*8;
	}
	else {
		control_node_data_requests_b += HEADER_SIZE + pkt.address_size*8;
	}
}

void log_cwritereq(Packet pkt) {
	control_node_write_requests++;
	// control_node_write_requests_b += HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8;
	if ((HEADER_SIZE + pkt.address_size*8) < 64*8) {
		control_node_data_requests_b += 64*8;
	}
	else {
		control_node_write_requests_b += pkt.address_size*8 + HEADER_SIZE;
	}
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
	// printf("Entered cnode process packet.\n");
	Packet ret_pkt;
	ret_pkt.flag = ERROR;
	ret_pkt.address_size = pkt.address_size;
	ret_pkt.data_size = pkt.data_size;


	if (pkt.flag == INVALIDATE) {
		// printf("Node %d received invalidation. Now in invalid.\n", pkt.dst);
		if (calculate_packet_size(INVALIDATE, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
			double num_pkts = calculate_packet_size(INVALIDATE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
			invalidation_msg += (int) CEIL(num_pkts);
			invalidation_b += CEIL(num_pkts) * MAX_PKT_SIZE*8;
		}
		else {
			invalidation_msg++;
			invalidation_b += MAX_PKT_SIZE * 8;//HEADER_SIZE + pkt.address_size*8;
		}
		*stall = 0;
		int i;
		for (i = 0; i < CACHE_LINES; i++) {
			if (node->cache[i].address == pkt.data.addr) {
				// printf("Node %d received invalidation, invalidating index %d.\n", node->id, i);
				break;
			}
		}
		// printf("I = %i\n", i);
		node->cache[i].state = INVALID;
		node->cache[i].valid = 0;
	}
	else if (pkt.flag == RESPONSE) {
		// printf("Node %d received data from a memory node for address %x. Now in exclusive for index %d.\n", node->id, pkt.data.addr, node->idx_to_modify);
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
		if (calculate_packet_size(STATECHANGE, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
			double num_pkt_to_represent = calculate_packet_size(STATECHANGE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
			state_change_ctrls +=  (int) CEIL(num_pkt_to_represent);
			state_change_ctrl_bits += (int) CEIL(num_pkt_to_represent) * MAX_PKT_SIZE*8;
			// printf("%d\n", (int) CEIL(num_pkt_to_represent));
		}
		else {
			state_change_ctrls++;
			state_change_ctrl_bits += MAX_PKT_SIZE*8; //HEADER_SIZE + pkt.address_size*8;
		}
		// fprintf(log_file, "Node %d received response for address 0x%lx at time %d\n\n\n", node->id, pkt.data.addr, global_time);
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
		// if (idxWithData >= CACHE_LINES) {
		// 	printf("Data not found!!!\n");
		// 	return ret_pkt;
		// }
		ret_pkt.id = 0;
		ret_pkt.time = 0;
		ret_pkt.flag = WR_DATA;
		ret_pkt.src = node->id;
		ret_pkt.dst = pkt.src;
		ret_pkt.address_size = pkt.address_size;
		ret_pkt.data_size = pkt.data_size;
		if (ret_pkt.src == ret_pkt.dst) {
			ret_pkt.flag = ERROR;
			return ret_pkt;
		}
		ret_pkt.data.addr = pkt.data.addr;
		// printf("Accessing node->cache[%d], wanting to have searched for %x in node %d\n", idxWithData, pkt.data.addr, node->id);
		ret_pkt.data.data = node->cache[idxWithData].value;
		// TODO: state change depending on the current state
		if (pkt.data.data == 0) {
			// printf("Node %d told to transfer to node %d. Now in owned.\n", node->id, pkt.src);
			if (node->cache[idxWithData].state != EXCLUSIVE) {
				if (calculate_packet_size(STATECHANGE, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
					double num_pkt_to_represent = calculate_packet_size(STATECHANGE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
					state_change_ctrls +=  (int) CEIL(num_pkt_to_represent);
					state_change_ctrl_bits += (int) CEIL(num_pkt_to_represent) * MAX_PKT_SIZE * 8;
					// printf("%d\n", (int) CEIL(num_pkt_to_represent));
				}
				else {
					state_change_ctrls++;
					state_change_ctrl_bits += MAX_PKT_SIZE*8;//HEADER_SIZE + pkt.address_size*8;
				}
			}
			node->cache[idxWithData].state = OWNED;
		}
		else if (pkt.data.data == 1) {
			// printf("Node %d told to transfer to node %d. Now in shared.\n", node->id, pkt.src);
			if (node->cache[idxWithData].state != SHARED) {
				if (calculate_packet_size(STATECHANGE, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
					double num_pkt_to_represent = calculate_packet_size(STATECHANGE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
					state_change_ctrls +=  (int) CEIL(num_pkt_to_represent);
					state_change_ctrl_bits += (int) CEIL(num_pkt_to_represent) * MAX_PKT_SIZE * 8;
					// printf("%d\n", (int) CEIL(num_pkt_to_represent));
				}
				else {
					state_change_ctrls++;
					state_change_ctrl_bits += MAX_PKT_SIZE*8;//HEADER_SIZE + pkt.address_size*8;
				}
			}
			node->cache[idxWithData].state = SHARED;
		}
		control_node_data_returns_to_cnodes++;
		if ((HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8) < 64*8) {
			compute_to_compute_b += 64*8;
		}
		else {
			compute_to_compute_b += HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8;
		}
		// log_cwritedata();
	}
	else if (pkt.flag == WR_DATA) {
		// if (node->id == 127) {}
		// printf("Node %d received data from node %d. Now in shared.\n", node->id, pkt.src);;
		node->cache[node->idx_to_modify].valid = 1;
		node->cache[node->idx_to_modify].state = SHARED;
		if (calculate_packet_size(STATECHANGE, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
			double num_pkt_to_represent = calculate_packet_size(STATECHANGE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
			state_change_ctrls +=  (int) CEIL(num_pkt_to_represent);
			state_change_ctrl_bits += (int) CEIL(num_pkt_to_represent) * MAX_PKT_SIZE * 8;
		}
		else {
			state_change_ctrls++;
			state_change_ctrl_bits += MAX_PKT_SIZE*8;//HEADER_SIZE + pkt.address_size*8;
		}
		node->cache[node->idx_to_modify].address = pkt.data.addr;
		node->cache[node->idx_to_modify].value = pkt.data.data;
		// fprintf(log_file, "Node %d received response for address 0x%lx at time %d\n\n\n", node->id, pkt.data.addr, global_time);
	}
	return ret_pkt;

}

void get_statistics()
{
	printf("\n\nInvalidation Messages from switch: %ld\n", invalidation_msg);
	printf("Invalidations from memory to switch: %ld\n", get_memory_to_switch_invalidations());
	printf("Read requests to memory nodes: %ld\n", control_node_data_requests); // not split
	printf("Write requests to memory nodes: %ld\n", control_node_write_requests); // not split
	printf("Transfer requests from memory nodes: %ld\n", transfer_requests()); // split
	printf("State change requests from memory node: %ld\n", state_change_ctrls); // split
	printf("Compute node to compute node data transfers: %ld\n", control_node_data_returns_to_cnodes); // not split
	printf("Compute node to memory node data transfers: %ld\n", control_node_data_returns); // not split
	printf("Memory node to compute node data transfer: %ld\n", get_memory_control_count()); // not split


	printf("\n\n\nPkt Sizes (KB):\n");
	printf("Invalidations from switch: %lf\n", invalidation_b / 8 / 1000);
	printf("Invalidations from memory nodes: %lf\n", get_mts_invalidations_b() / 8 / 1000);
	printf("Read requests: %lf\n", control_node_data_requests_b / 8 / 1000);
	printf("Write requests: %lf\n", control_node_write_requests_b / 8 / 1000);
	printf("Transfer requests: %lf\n", transfer_requests_b() / 8 / 1000);
	// printf("State change requests: %lf\n", 0.0);
	printf("Compute -> Compute transfers: %lf\n", compute_to_compute_b / 8 / 1000);
	printf("Compute -> Memory transfers: %lf\n", compute_to_memory_b / 8 / 1000);
	printf("Memory -> Compute transfers: %lf\n", mem_to_compute_b() / 8 / 1000);

	printf("\n\n\n\nDistributions (KB)\n");
	printf("Compute->Switch Memory: %lf\n", (double) (control_node_data_requests_b + control_node_write_requests_b + compute_to_compute_b + compute_to_memory_b) / 8 / 1000);
	printf("Compute->Switch Coherence: 0\n\n");

	printf("Switch->Compute Memory: %lf\n", (double) (mem_to_compute_b() + compute_to_compute_b) / 8 / 1000);
	// TODO: modify state_change_ctrls to be data sizes instead
	printf("Switch->Compute Coherence: %lf\n\n", (double) (invalidation_b + transfer_requests_b() + state_change_ctrl_bits) / 8 / 1000);

	printf("Memory->Switch Memory: %lf\n", (double) (mem_to_compute_b()) / 8 / 1000);
	// TODO: modify state_change_ctrls to be data sizes instead
	printf("Memory->Switch Coherence: %lf\n\n", (double) (get_mts_invalidations_b() + transfer_requests_b() + state_change_ctrl_bits) / 8 / 1000);

	printf("Switch->Memory Memory: %lf\n", (double) (control_node_data_requests_b + control_node_write_requests_b + compute_to_memory_b) / 8 / 1000);
	printf("Switch->Memory Coherence: %lf\n", 0.0);


}