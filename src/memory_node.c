#include "stdint.h"

#include "memory_node.h"
#include "packet.h"

Packet process_packet(MemoryNode* node, Packet pkt, uint32_t global_id, uint32_t global_time, uint32_t memory_node_min_id)
{
	Packet return_packet;
	return_packet.id = global_id;
	return_packet.time = global_time + 1; // queue for next global time
	return_packet.flag = ERROR;
	return_packet.src = node->id;
	return_packet.dst = pkt.src;
	uint32_t address_to_access = (pkt.data.addr >> 3) - (64 * (node->id - memory_node_min_id)); // need to figure out which memory block this is to get correct line

	if (pkt.flag == READ)
	{
		return_packet.flag = NORMAL;
		// uint32_t address_to_access = (pkt.data.addr >> 3) - (64 * (node->id - memory_node_min_id)); // need to figure out which memory block this is to get correct line
		DataNode return_data = { pkt.data.addr, node->memory[address_to_access].value };
		return_packet.data = return_data;
	}
	else if (pkt.flag == WRITE)
	{
		// TODO
		node->memory[address_to_access].value = pkt.data.data;
	}
	else if (pkt.flag == ERROR)
	{
		// TODO
	}

	return return_packet;
}