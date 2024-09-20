#include "stdint.h"

#include "memory_node.h"
#include "packet.h"
#include <stdio.h>
#include "compute_node.h"

long control_message_memory_node_global_counter = 0;
long transfer_command_messages = 0;
long mem_to_switch_invalidations = 0;
long memory_data_requests = 0;
int data_requested;
long last_sent_node = -1;
long last_sent_address = -1;
long last_sent_invalidate = -1;
long last_acted_node = -1;

void init_memnodes(MemoryNode* node, int node_cnt) {
	for (int i = 0; i < MEM_NUM_LINES; i++) {
		for (int k = 0; k < node_cnt; k++) {
			node->memory[i].nodeState[k] = INVALID;
		}
	}
}

Packet process_packet(MemoryNode* node, Packet pkt, uint32_t global_id, uint32_t global_time, Port* p)
{
	Packet return_packet;
	return_packet.id = global_id;
	return_packet.time = global_time + 1; // queue for next global time
	return_packet.flag = ERROR;
	return_packet.src = node->id;
	return_packet.dst = pkt.src;
	// uint32_t address_to_access = (pkt.data.addr >> 3) - (64 * (node->id - memory_node_min_id)); // need to figure out which memory block this is to get correct line
	uint64_t address_to_access = (pkt.data.addr / 4); // % 64;
	// printf("Address 0x%lx\n", pkt.data.addr);
	int existsinmem = 0;
	// printf("Searching for address 0x%llx\n", pkt.data.addr);
	for (int i = 0; i < MEM_NUM_LINES; i++) {
		// printf("Checking index %d.\n", i);
		if (node->memory[i].address == pkt.data.addr) {
			// printf("Address %lx exists in memory.\n", pkt.data.addr);
			existsinmem = 1;
			address_to_access = i;
			// printf("Address present.\n");
		}
	}
	// printf("Address present? %d\n", existsinmem);
	// printf("Accessing index %d to determine address %llx.\n", address_to_access, pkt.data.addr);
	if (!existsinmem ) {
		printf("nonpresent address 0x%lx 0d%ld for write flag %d\n", pkt.data.addr, pkt.data.addr, 1 ? pkt.flag == WR_REQUEST : READ_REQUEST);
		// return return_packet;
	}
	// printf("Accessing index %d for address 0x%lx.\n", address_to_access, pkt.data.addr);
	// printf("Trying to access address: 0x%lx in node: %d\n", pkt.data.addr, node->id - memory_node_min_id);
	// printf("Resultant index: %d\n", address_to_access);
	if (pkt.flag == READ_REQUEST)
	{
		// Check if any other node should be sending data to the requestor
		// printf("Received read request from node %d.\n", pkt.src);
		int modifiedElsewhere = 0;
		int exclusiveElsewhere = 0;
		int ownedElsewhere = 0;
		int sharedElsewhere = 0;
		int elseNode;
		for (int i = 0; i < 128; i++) {
			if (node->memory[address_to_access].nodeState[i] == MODIFIED) {
				modifiedElsewhere = 1;
				elseNode = i;
			}
			if (node->memory[address_to_access].nodeState[i] == EXCLUSIVE) {
				exclusiveElsewhere = 1;
				elseNode = i;
			}
			if (node->memory[address_to_access].nodeState[i] == SHARED) {
				sharedElsewhere = 1;
				elseNode = i;
			}
			if (node->memory[address_to_access].nodeState[i] == OWNED) {
				ownedElsewhere = 1;
				elseNode = i;
			}

		}

		if (modifiedElsewhere) { // send a transfer packet
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 0; // means go to owned
			node->memory[address_to_access].nodeState[elseNode] = OWNED;
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			transfer_command_messages++;
			// printf("Modified in node %d\n", elseNode);
			// TODO: send a state change packet to OWNED
		}
		else if (exclusiveElsewhere) {
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 1; // means go to shared
			node->memory[address_to_access].nodeState[elseNode] = SHARED;
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			transfer_command_messages++;
			// printf("Exclusive in node %d\n", elseNode);
			// TODO: send a state change packet to SHARED
		}
		else if (ownedElsewhere) {
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 0; // means stay in owned
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			transfer_command_messages++;
			// printf("Owned in node %d\n");
		}
		else if (sharedElsewhere) {
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 1; // means stay in shared
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			transfer_command_messages++;
		}
		// Memory's job to update the requestor
		else {
			return_packet.flag = RESPONSE;
			return_packet.src = node->id;
			return_packet.dst = pkt.src;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = node->memory[address_to_access].value;
			control_message_memory_node_global_counter++;
			node->memory[address_to_access].nodeState[pkt.src] = EXCLUSIVE;
		}
	}
	else if (pkt.flag == WR_REQUEST) { // write request from a compute node
		// printf("received write request from %d.\n", pkt.src);
		node->memory[address_to_access].nodeState[pkt.src] = MODIFIED;
		// printf("Memory is generating invalidations.\n");
		return_packet.dst = 0;
		return_packet.invalidates = malloc(sizeof(uint8_t) * 128);
		return_packet.data.addr = pkt.data.addr;
		int sendInvalidations = 0;
		int invalidation_num = 0;

		// For 128 nodes, we need 5 invalidation packets to make sure we don't go over IPG packet size
		Packet invalidate_packet_0 = (Packet) {global_id, global_time, INVALIDATE, node->id, 0, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL};
		invalidate_packet_0.invalidates = malloc(sizeof(uint8_t) * 46);
		Packet invalidate_packet_1 = (Packet) {global_id, global_time, INVALIDATE, node->id, 0, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL};
		invalidate_packet_1.invalidates = malloc(sizeof(uint8_t) * 46);
		Packet invalidate_packet_2 = (Packet) {global_id, global_time, INVALIDATE, node->id, 0, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL};
		invalidate_packet_2.invalidates = malloc(sizeof(uint8_t) * 46);
		// Packet invalidate_packet_3 = (Packet) {global_id, global_time, INVALIDATE, node->id, 0, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL};
		// invalidate_packet_3.invalidates = malloc(sizeof(uint8_t) * 27);
		// Packet invalidate_packet_4 = (Packet) {global_id, global_time, INVALIDATE, node->id, 0, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL};
		// invalidate_packet_4.invalidates = malloc(sizeof(uint8_t) * 27);

		for (int i = 0; i < 128; i++) {
			// Split packet into 5 packets
			if (i >= 0 && i <= 45) {
				invalidation_num = 0;
			}
			if (i >= 46 && i <= 91) {
				invalidation_num = 1;
			}
			if (i >= 92 && i <= 127) {
				invalidation_num = 2;
			}
			// if (i >= 81 && i <= 107) {
			// 	invalidation_num = 3;
			// }
			// if (i >= 108 && i <= 127) {
			// 	invalidation_num = 4;
			// }


			int invalidate_i = 0;
			if (node->memory[address_to_access].nodeState[i] != INVALID && i != pkt.src) {
				// printf("Node %i not in invalid.\n", i);
				// return_packet.invalidates[i] = 1;
				invalidate_i = 1;
				node->memory[address_to_access].nodeState[i] = INVALID;
				sendInvalidations = 1;
				// printf("Setting invalidations for %d.\n", i);
			}

			
			if (invalidate_i) {
				switch (invalidation_num) {
					case 0:
						invalidate_packet_0.invalidates[i] = 1;
						break;
					case 1:
						invalidate_packet_1.invalidates[i - 46] = 1;
						break;
					case 2:
						invalidate_packet_2.invalidates[i - 92] = 1;
						break;
					// case 3:
					// 	invalidate_packet_3.invalidates[i - 81] = 1;
					// 	break;
					// case 4: 
					// 	invalidate_packet_4.invalidates[i - 108] = 1;
					// 	break;
				}
			}
			// else {
			// 	return_packet.invalidates[i] = 0;
			// }
		}
		if (sendInvalidations) {
			mem_to_switch_invalidations += 3; // 5 Packets in order to limit to 6 byte packets
			push_packet(p, TX, invalidate_packet_0);
			push_packet(p, TX, invalidate_packet_1);
			push_packet(p, TX, invalidate_packet_2);
			// push_packet(p, TX, invalidate_packet_3);
			// push_packet(p, TX, invalidate_packet_4);
			return_packet.flag = ERROR; // we don't want the returned packet to be put anywhere, so let main discard it for us
		}
		// generate_invalidations(node, pkt, p, global_id, global_time);
	}
	else if (pkt.flag == WR_DATA) { 
		// printf("Received a writeback packet from Node %d.\n", pkt.src);
		node->memory[address_to_access].nodeState[pkt.src] = INVALID;
		node->memory[address_to_access].value = pkt.data.data;

	}
	return return_packet;
}

void generate_invalidations(MemoryNode* node, Packet pkt, Port* p, uint32_t global_id, uint32_t global_time) {
	uint32_t idx_to_access = (pkt.data.addr / 4);// % 64;
	for (int i = 0; i < 128; i++) {
		// send an invalidation to ever non modified node
		if (node->memory[idx_to_access].nodeState[i] != INVALID && i != pkt.src) {
			// printf("Due to write from Compute node %d, Invalidating Compute Node %d\n", pkt.src, i);
			// invalidate
			// Packet new_packet;
			// new_packet.id = global_id;
			// new_packet.time = global_time + 1;
			// new_packet.flag = INVALIDATE; 
			// new_packet.src = node->id;
			// new_packet.dst = i;
			Packet invalidate_packet = (Packet) {global_id, global_time, INVALIDATE, node->id, i, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL};
			push_packet(p, TX, invalidate_packet);
			node->memory[idx_to_access].nodeState[i] = INVALID;
		}
	}
}

long get_memory_control_count()
{
	return control_message_memory_node_global_counter;
}

long transfer_requests() {
	return transfer_command_messages;
}

long get_memory_to_compute_requests() {
	return memory_data_requests;
}

long get_memory_to_switch_invalidations() {
	return mem_to_switch_invalidations;
}