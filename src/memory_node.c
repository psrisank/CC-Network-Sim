#include "stdint.h"

#include "memory_node.h"
#include "packet.h"
#include <stdio.h>
#include "compute_node.h"
// #include <math.h>

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))
#define CEILING_NEG(X) (int)(X)
#define CEIL(X) ( ((X) > 0) ? CEILING_POS(X) : CEILING_NEG(X) )

long control_message_memory_node_global_counter = 0;
double control_message_memory_node_global_counter_b = 0;
long transfer_command_messages = 0;
double transfer_command_messages_b = 0;

long mem_to_switch_invalidations = 0;
double mts_invalidations_b;

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
	return_packet.address_size = pkt.address_size;
	return_packet.data_size = pkt.data_size;
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
		printf("nonpresent address 0x%lx 0d%ld for write flag %d\n", pkt.data.addr, pkt.data.addr, pkt.flag == WR_REQUEST ? 1 : 0);
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
			return_packet.address_size = pkt.address_size;
			return_packet.data_size = 0;

			node->memory[address_to_access].nodeState[elseNode] = OWNED;
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			// printf("Transferring...\n");
			if (calculate_packet_size(TRANSFER, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
				double num_pkt_to_represent = calculate_packet_size(TRANSFER, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
				transfer_command_messages +=  (int) CEIL(num_pkt_to_represent);
				transfer_command_messages_b += CEIL(num_pkt_to_represent) * MAX_PKT_SIZE*8;
			}
			else {
				transfer_command_messages++;
				transfer_command_messages_b += MAX_PKT_SIZE*8; //HEADER_SIZE + pkt.address_size*8;
			}
			// transfer_command_messages++;
			// printf("Modified in node %d\n", elseNode);
			// TODO: send a state change packet to OWNED
		}
		else if (exclusiveElsewhere) {
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 1; // means go to shared
			return_packet.address_size = pkt.address_size;
			node->memory[address_to_access].nodeState[elseNode] = SHARED;
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			// printf("Transferring...\n");

			if (calculate_packet_size(TRANSFER, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
				double num_pkt_to_represent = calculate_packet_size(TRANSFER, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
				transfer_command_messages +=  (int) CEIL(num_pkt_to_represent);
				transfer_command_messages_b += CEIL(num_pkt_to_represent) * MAX_PKT_SIZE*8;
			}
			else {
				transfer_command_messages++;
				transfer_command_messages_b +=  MAX_PKT_SIZE*8; //HEADER_SIZE + pkt.address_size*8;
			}
			// printf("Exclusive in node %d\n", elseNode);
			// TODO: send a state change packet to SHARED
		}
		else if (ownedElsewhere) {
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 0; // means stay in owned
			return_packet.address_size = pkt.address_size;
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			// printf("Transferring...\n");

			if (calculate_packet_size(TRANSFER, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
				double num_pkt_to_represent = calculate_packet_size(TRANSFER, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
				transfer_command_messages +=  (int) CEIL(num_pkt_to_represent);
				int new = (int) CEIL(num_pkt_to_represent);
				transfer_command_messages_b += CEIL(num_pkt_to_represent) * MAX_PKT_SIZE*8;
				// printf("Generated %d messages for an invalidation from mem node to switch.\n", new);
			}
			else {
				transfer_command_messages++;
				transfer_command_messages_b += MAX_PKT_SIZE*8; //HEADER_SIZE + pkt.address_size*8;
			}
			// printf("Owned in node %d\n");
		}
		else if (sharedElsewhere) {
			return_packet.flag = TRANSFER;
			return_packet.src = pkt.src;
			return_packet.dst = elseNode;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = 1; // means stay in shared
			return_packet.address_size = pkt.address_size;
			node->memory[address_to_access].nodeState[pkt.src] = SHARED;
			// printf("Transferring...\n");

			if (calculate_packet_size(TRANSFER, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
				double num_pkt_to_represent = calculate_packet_size(TRANSFER, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
				transfer_command_messages +=  (int) CEIL(num_pkt_to_represent);
				transfer_command_messages_b += CEIL(num_pkt_to_represent) * MAX_PKT_SIZE * 8;
			}
			else {
				transfer_command_messages++;
				transfer_command_messages_b += MAX_PKT_SIZE*8; // + pkt.address_size*8;
			}
		}
		// Memory's job to update the requestor
		else {
			return_packet.flag = RESPONSE;
			return_packet.src = node->id;
			return_packet.dst = pkt.src;
			return_packet.data.addr = pkt.data.addr;
			return_packet.data.data = node->memory[address_to_access].value;
			control_message_memory_node_global_counter++;
			if ((HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8) < 64) {
				control_message_memory_node_global_counter_b += 64*8;
			}
			else {
				control_message_memory_node_global_counter_b += HEADER_SIZE + pkt.address_size*8 + pkt.data_size*8;
			}
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

		if (MULTICAST) { // multicasting, so send one packet with all destinations
			for (int i = 0; i < 128; i++) {
				if (node->memory[address_to_access].nodeState[i] != INVALID && i != pkt.src) {
					// printf("Node %i not in invalid.\n", i);
					return_packet.invalidates[i] = 1;
					node->memory[address_to_access].nodeState[i] = INVALID;
					sendInvalidations = 1;
					// printf("Setting invalidations for %d.\n", i);
				}
				else {
					return_packet.invalidates[i] = 0;
				}
			}
			if (sendInvalidations) {
				return_packet.flag = INVALIDATE;
				if (calculate_packet_size(INVALIDATE, pkt.address_size, 0) > (double) MAX_PKT_SIZE) {
					double num_pkt_to_represent = calculate_packet_size(INVALIDATE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
					// new = (int) CEIL(num_pkt_to_represent);
					mem_to_switch_invalidations += (int) CEIL(num_pkt_to_represent);
					mts_invalidations_b += CEIL(num_pkt_to_represent) * MAX_PKT_SIZE * 8;
				}
				else {
					mem_to_switch_invalidations++;
					// new = 1;
					mts_invalidations_b += HEADER_SIZE + pkt.address_size*8;
				}
			}		
			// printf("Generated %d messages for an invalidation.\n", new);
		}
		else {
			generate_invalidations(node, pkt, p, global_id, global_time, address_to_access);
			return_packet.flag = ERROR; // ignore packet that we generated.
		}
	}
	else if (pkt.flag == WR_DATA) { 
		// printf("Received a writeback packet from Node %d.\n", pkt.src);
		node->memory[address_to_access].nodeState[pkt.src] = INVALID;
		node->memory[address_to_access].value = pkt.data.data;

	}
	else if (pkt.flag == EVICTION) {
		// printf("Received an eviction packet from node %d.\n", pkt.src);
		node->memory[address_to_access].value = pkt.data.data;
		node->memory[address_to_access].nodeState[pkt.src] = INVALID;
		// node->memory[address_to_access].value = 0xBEEFDEAD;
	}
	return return_packet;
}

void generate_invalidations(MemoryNode* node, Packet pkt, Port* p, uint32_t global_id, uint32_t global_time, uint64_t address_to_access) {
	// uint32_t idx_to_access = (pkt.data.addr / 4);// % 64;
	for (int i = 0; i < 128; i++) {
		// send an invalidation to ever non modified node
		if (node->memory[address_to_access].nodeState[i] != INVALID && i != pkt.src) {
			Packet invalidate_packet = (Packet) {global_id, global_time, INVALIDATE, node->id, i, (DataNode) {pkt.data.addr, 0xFFFFFFFF}, NULL, pkt.address_size, pkt.data_size};
			push_packet(p, TX, invalidate_packet);
			node->memory[address_to_access].nodeState[i] = INVALID;
			if (calculate_packet_size(INVALIDATE, pkt.address_size, 0) > MAX_PKT_SIZE) {
				double num_pkt_to_represent = calculate_packet_size(INVALIDATE, pkt.address_size, 0) / (double) MAX_PKT_SIZE;
				mem_to_switch_invalidations += (int) CEIL(num_pkt_to_represent);
				mts_invalidations_b += CEIL(num_pkt_to_represent) * MAX_PKT_SIZE * 8;
			}
			else {
				mem_to_switch_invalidations++;
				mts_invalidations_b += MAX_PKT_SIZE;
			}
		}
	}
}

long get_memory_control_count()
{
	return control_message_memory_node_global_counter;
}

double mem_to_compute_b() {
	return control_message_memory_node_global_counter_b;
}

long transfer_requests() {
	return transfer_command_messages;
}

double transfer_requests_b() {
	return transfer_command_messages_b;
}

long get_memory_to_compute_requests() {
	return memory_data_requests;
}

long get_memory_to_switch_invalidations() {
	return mem_to_switch_invalidations;
}

double get_mts_invalidations_b() {
	return mts_invalidations_b;
}