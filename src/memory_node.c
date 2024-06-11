#include "stdint.h"

#include "memory_node.h"
#include "packet.h"
#include <stdio.h>
#include "compute_node.h"

int control_message_memory_node_global_counter = 0;
int memory_data_requests = 0;
int data_requested;
long last_sent_node = -1;
long last_sent_address = -1;
long last_sent_invalidate = -1;
long last_acted_node = -1;

void init_memnodes(MemoryNode* node, int node_cnt) {
	for (int i = 0; i < MEM_NUM_LINES; i++) {
		for (int k = 0; k < node_cnt; k++) {
			node->memory[i].nodeState[k] = I;
		}
	}
}

Packet process_packet(MemoryNode* node, Packet pkt, uint32_t global_id, uint32_t global_time, uint32_t memory_node_min_id, Port* p)
{
	Packet return_packet;
	return_packet.id = global_id;
	return_packet.time = global_time + 1; // queue for next global time
	return_packet.flag = ERROR;
	return_packet.src = node->id;
	return_packet.dst = pkt.src;
	// uint32_t address_to_access = (pkt.data.addr >> 3) - (64 * (node->id - memory_node_min_id)); // need to figure out which memory block this is to get correct line
	uint32_t address_to_access = (pkt.data.addr / 4) % 64;
	// printf("Trying to access address: 0x%lx in node: %d\n", pkt.data.addr, node->id - memory_node_min_id);
	// printf("Resultant index: %d\n", address_to_access);
	if (pkt.flag == READ)
	{
		// printf("Received a read from %d\n", pkt.src);
		// printf("Last sent node;")
		// printf("Data was requested already? %d.\n",data_requested);
		if (data_requested || (pkt.src == last_sent_node && pkt.data.addr == last_sent_address && last_acted_node == last_sent_node)) {
			// printf("Disregarding the read.\n");
			unlog_cdatareq();
			return return_packet;
		}

		int modifiedElsewhere = 0;
		int nodeModified;
		for (int i = 0; i < 128; i++) {
			if (node->memory[address_to_access].nodeState[i] == M) {
				modifiedElsewhere = 1;
				// printf("modified elsewhere\n");
				nodeModified = i;
				break; 
			}
		}
		if (modifiedElsewhere) {  // request data from the modified cache
			// Create a read packet
			// printf("Modified elsewhere.\n");
			data_requested = 1;
			Packet data_request;
			data_request.id = global_id;
			data_request.time = global_time + 1;
			data_request.flag = READ;
			data_request.src = node->id;
			data_request.dst = nodeModified;
			data_request.data.addr = pkt.data.addr;
			data_request.data.data = 0xFFFFFFF;
			push_packet(p, TX, data_request);
			// printf("Requested new data from compute node %d.\n", nodeModified);
			unlog_cdatareq(); // unlogging a data request since the node will send a data request again
			memory_data_requests++; // logging a data request from a control node
			return return_packet;
		}

		// printf("Sending return data for memory for address 0x%lx\n", (long unsigned int) pkt.data.addr);
		return_packet.flag = NORMAL;
		// uint32_t address_to_access = (pkt.data.addr >> 3) - (64 * (node->id - memory_node_min_id)); // need to figure out which memory block this is to get correct line
		DataNode return_data = { pkt.data.addr, node->memory[address_to_access].value };
		return_packet.data = return_data;
		// node->memory[address_to_access].nodeState[0] = S;
		(node->memory[address_to_access]).nodeState[pkt.src] = S;
		control_message_memory_node_global_counter++;
		last_sent_node = pkt.src;
		last_sent_address = pkt.data.addr;
		last_acted_node = pkt.src;
	}
	else if (pkt.flag == WR_SIGNAL) // if a write request is received, retrieve the data from the modified cache
	{
		// printf("Received a write signal.\n");
		// TODO
		last_acted_node = pkt.src;
		int modifiedElsewhere = 0;
		int nodeModified;
		for (int i = 0; i < 128; i++) {
			if (node->memory[address_to_access].nodeState[i] == M) {
				modifiedElsewhere = 1;
				nodeModified = i;
				break; 
			}
		}

		// if (modifiedElsewhere) {  // request data from the modified cache
		// 	// Create a read packet
		// 	// printf("Address was found to be written to elsewhere, mainly node %d.\n", nodeModified);
		// 	data_requested = 1;
		// 	// Packet data_request;
		// 	// data_request.id = global_id;
		// 	// data_request.time = global_time + 1;
		// 	// data_request.flag = READ;
		// 	// data_request.src = node->id;
		// 	// data_request.dst = nodeModified;
		// 	// data_request.data.addr = pkt.data.addr;
		// 	// data_request.data.data = 0xFFFFFFF;
		// 	// push_packet(p, TX, data_request);
		// 	Packet invalidate_packet = (Packet) {global_id, global_time, INVALIDATE, node->id, nodeModified, (DataNode) {pkt.data.addr, 0xFFFFFFFF}};
		// 	push_packet(p, TX, invalidate_packet);
		// 	last_sent_invalidate = nodeModified;
		// }
		// else { // invalidated the shared caches
			// No packet necessary, just invalidate other caches
			node->memory[address_to_access].nodeState[pkt.src] = M;
			generate_invalidations(node, pkt, p, global_id, global_time, memory_node_min_id);
			for (int i = 0; i < 128; i++) {
				if (i != pkt.src) {
					node->memory[address_to_access].nodeState[i] = I;
				}
			}	
		// }
	}
	else if (pkt.flag == WR_DATA) {
		// printf("Received a write from compute node %d.\n", pkt.src);
		node->memory[address_to_access].value = pkt.data.data;
		node->memory[address_to_access].nodeState[pkt.src] = S;
		data_requested = 0;
		if (last_sent_invalidate == pkt.src) {
			node->memory[address_to_access].nodeState[pkt.src] = I;
		}
	}
	else if (pkt.flag == ERROR)
	{
		// TODO
	}

	// printf("Status of address 0x%lx:\n", pkt.data.addr);
	// for (int i = 0; i < 128; i++) {
	// 	printf("%d\t",i);
	// }
	// printf("\n");
	// for (int i = 0 ; i < 128; i++) {
	// 	printf("%d\t", node->memory[address_to_access].nodeState[i]);
	// }
	// printf("\n");
	return return_packet;
}

void generate_invalidations(MemoryNode* node, Packet pkt, Port* p, uint32_t global_id, uint32_t global_time, uint32_t memory_node_min_id) {
	uint32_t idx_to_access = (pkt.data.addr / 4) % 64;
	for (int i = 0; i < 128; i++) {
		// send an invalidation to ever non modified node
		if (node->memory[idx_to_access].nodeState[i] == S || (node->memory[idx_to_access].nodeState[i] == M && i != pkt.src)) {
			// printf("Due to write from Compute node %d, Invalidating Compute Node %d\n", pkt.src, i);
			// invalidate
			// Packet new_packet;
			// new_packet.id = global_id;
			// new_packet.time = global_time + 1;
			// new_packet.flag = INVALIDATE; 
			// new_packet.src = node->id;
			// new_packet.dst = i;
			Packet invalidate_packet = (Packet) {global_id, global_time, INVALIDATE, node->id, i, (DataNode) {pkt.data.addr, 0xFFFFFFFF}};
			push_packet(p, TX, invalidate_packet);
		}
	}
}

int get_memory_control_count()
{
	return control_message_memory_node_global_counter;
}

int get_memory_to_compute_requests() {
	return memory_data_requests;
}