#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "main.h"
#include "compute_node.h"
#include "switch_node.h"
#include "memory_node.h"
#include "packet.h"
#include "port.h"

int main()
{
	// TODO parse input file here

	// universal variables
	uint32_t global_time = 0;
	uint32_t global_id = 1; // skip 0 for debugging purposes later

	// compute nodes
	ComputeNode compute_nodes[NUM_COMPUTE_NODES];
	uint32_t compute_node_min_id = global_id;
	for (int i = 0; i < NUM_COMPUTE_NODES; i++)
	{
		ComputeNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < CTRL_NUM_BOT_PORTS; j++)
		{
			node.bot_ports[j].tail_tx = 0;
			node.bot_ports[j].tail_rx = 0;
		}
		compute_nodes[i] = node;
	}
	uint32_t compute_node_max_id = global_id - 1;

	// switch nodes
	SwitchNode switch_nodes[NUM_SWITCH_NODES];
	uint32_t switch_mode_min_id = global_id;
	for (int i = 0; i < NUM_SWITCH_NODES; i++)
	{
		SwitchNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
		{
			node.top_ports[j].tail_tx = 0;
			node.top_ports[j].tail_rx = 0;
		}
		for (int j = 0; j < SW_NUM_BOT_PORTS; j++)
		{
			node.bot_ports[j].tail_tx = 0;
			node.bot_ports[j].tail_rx = 0;
		}
		switch_nodes[i] = node;
	}
	uint32_t switch_node_max_id = global_id - 1;

	// memory nodes
	MemoryNode memory_nodes[NUM_MEMORY_NODES];
	uint32_t memory_node_min_id = global_id;
	for (int i = 0; i < NUM_MEMORY_NODES; i++)
	{
		MemoryNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < MEM_NUM_TOP_PORTS; j++)
		{
			node.top_ports[j].tail_tx = 0;
			node.top_ports[j].tail_rx = 0;
		}
		memory_nodes[i] = node;
	}
	uint32_t memory_node_max_id = global_id - 1;

	// TODO TESTING REMOVE
	// create test data for packet
	DataNode data_node = { 0x00000000, 0x00000000 };
	// create packet with data
	uint8_t dest = ((data_node.addr >> 2) / MEM_NUM_LINES) + memory_node_min_id; // TODO SAVE THIS
	Packet test_packet = { global_id++, global_time, READ, compute_nodes[0].id, dest, data_node };
	// place a packet in compute node destined to memory node
	if (push_packet((&(compute_nodes[0].bot_ports[0])), TX, test_packet) != 0)
	{
		return EXIT_FAILURE;
	}
	print_port(compute_nodes[0].bot_ports[0], TX);
	// initialize memory location
	MemoryLine line = { 0x00000000, 0xDEADBEEF, SHARED };
	memory_nodes[0].memory[0] = line;

	// main loop
	for (; global_time < 10;) // TODO loop through packets to send
	{
		printf("--- GLOBAL TIME = %d\n", global_time);
		// outgoing from compute nodes
		for (int i = 0; i < NUM_COMPUTE_NODES; i++)
		{
			printf("* Compute node with ID %d at time %d\n", compute_nodes[i].id, global_time);
			// loop through outgoing ports to switch nodes
			for (int j = 0; j < CTRL_NUM_BOT_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(compute_nodes[i].bot_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(compute_nodes[i].bot_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= memory_node_min_id) && (curr_packet_tx.dst <= memory_node_max_id))
					{
						printf("Moving packet with ID %d to switch with ID %d\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place packet into switch
						push_packet((&(switch_nodes[0].top_ports[i])), RX, curr_packet_tx);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					}
					// remove packet from compute node
					pop_packet((&(compute_nodes[i].bot_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if (curr_packet_rx.dst == compute_nodes[i].id)
					{
						printf("Packet with ID %d has arrived at compute node with ID %d [0x%08x, 0x%08x]\n", curr_packet_tx.id, compute_nodes[i].id, curr_packet_rx.data.addr, curr_packet_rx.data.data);
						// TODO act on this
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
					}
					// remove packet from compute node
					pop_packet((&(compute_nodes[i].bot_ports[j])), RX, 1);
				}
			}
		}

		// outgoing from switch nodes
		for (int i = 0; i < NUM_SWITCH_NODES; i++)
		{
			printf("* Switch node with ID %d at time %d\n", switch_nodes[i].id, global_time);
			// loop through outgoing ports to memory nodes
			for (int j = 0; j < SW_NUM_BOT_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(switch_nodes[i].bot_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(switch_nodes[i].bot_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= memory_node_min_id) && (curr_packet_tx.dst <= memory_node_max_id))
					{
						printf("Moving packet with ID %d to memory node with ID %d\n", curr_packet_tx.id, memory_nodes[i].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place packet into memory node
						push_packet((&(memory_nodes[i].top_ports[i])), RX, curr_packet_tx);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					}
					// remove packet from switch bottom output port
					pop_packet((&(switch_nodes[i].bot_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_rx.dst >= compute_node_min_id) && (curr_packet_rx.dst <= compute_node_max_id))
					{
						printf("Moving packet with ID %d to output queue\n", curr_packet_rx.id);
						curr_packet_rx.time += GLOBAL_TIME_INCR;
						// place packet into switch top output port
						push_packet((&(switch_nodes[0].top_ports[i])), TX, curr_packet_rx);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
					}
					// remove packet from switch bottom input port
					pop_packet((&(switch_nodes[i].bot_ports[j])), RX, 1);
				}
			}

			for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(switch_nodes[i].top_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(switch_nodes[i].top_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= compute_node_min_id) && (curr_packet_tx.dst <= compute_node_max_id))
					{
						printf("Moving packet with ID %d to compute node with ID %d\n", curr_packet_tx.id, compute_nodes[i].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place packet into compute node
						push_packet((&(compute_nodes[i].bot_ports[i])), RX, curr_packet_tx);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					}
					// remove packet from switch top output port
					pop_packet((&(switch_nodes[i].top_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_rx.dst >= memory_node_min_id) && (curr_packet_rx.dst <= memory_node_max_id))
					{
						printf("Moving packet with ID %d to output queue\n", curr_packet_rx.id);
						curr_packet_rx.time += GLOBAL_TIME_INCR;
						// place node into switch bottom output port
						push_packet((&(switch_nodes[0].bot_ports[i])), TX, curr_packet_rx);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
					}
					// remove packet from switch top input port
					pop_packet((&(switch_nodes[i].top_ports[j])), RX, 1);
				}
			}
		}

		// // outgoing from memory nodes
		for (int i = 0; i < NUM_MEMORY_NODES; i++)
		{
			printf("* Memory node with ID %d at time %d\n", memory_nodes[i].id, global_time);
			// loop through outgoing ports to switch nodes
			for (int j = 0; j < MEM_NUM_TOP_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(memory_nodes[i].top_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(memory_nodes[i].top_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= compute_node_min_id) && (curr_packet_tx.dst <= compute_node_max_id))
					{
						printf("Moving packet with ID %d to switch with ID %d\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place packet into switch
						push_packet((&(switch_nodes[0].bot_ports[i])), RX, curr_packet_tx);
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					}
					// remove packet from memory node
					pop_packet((&(memory_nodes[i].top_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if (curr_packet_rx.dst == memory_nodes[i].id)
					{
						printf("Packet with ID %d has arrived at memory node with ID %d [0x%08x, 0x%08x]\n", curr_packet_rx.id, memory_nodes[i].id, curr_packet_rx.data.addr, curr_packet_rx.data.data);
						// handle return packet
						Packet return_packet = process_packet(memory_nodes[i], curr_packet_rx, global_id++, global_time);
						if (return_packet.flag != ERROR)
						{
							// place packet in outgoing buffer
							push_packet((&(memory_nodes[i].top_ports[i])), TX, return_packet);
						}
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
					}
					// remove packet from memory node
					pop_packet((&(memory_nodes[i].top_ports[j])), RX, 1);
				}
			}
		}
		
		// end of loop
		global_time += GLOBAL_TIME_INCR;

		// return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}