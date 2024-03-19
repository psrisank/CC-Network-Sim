#include "stdint.h"

#include "memory_node.h"
#include "packet.h"

Packet process_packet(MemoryNode node, Packet pkt, uint32_t global_id, uint32_t global_time)
{
	Packet return_packet;
	return_packet.id = global_id;
	return_packet.time = global_time + 1; // queue for next global time
	return_packet.flag = ERROR;
	return_packet.src = node.id;
	return_packet.dst = pkt.src;

	if (pkt.flag == READ)
	{
		return_packet.flag = NORMAL;
		DataNode return_data = { pkt.data.addr, node.memory[pkt.data.addr >> 2].value };
		return_packet.data = return_data;
	}
	else if (pkt.flag == WRITE)
	{
		// TODO
	}
	else if (pkt.flag == ERROR)
	{
		// TODO
	}

	return return_packet;
}