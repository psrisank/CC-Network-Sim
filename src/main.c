#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "main.h"
#include "control_node.h"
#include "switch_node.h"
#include "memory_node.h"
#include "packet.h"

int main()
{
	// TODO parse input file here

	// universal variables
	uint32_t global_time = 0;
	uint32_t global_id = 1; // skip 0 for debugging purposes later

	// control nodes
	ControlNode control_nodes[NUM_CONTROL_NODES];
	uint32_t control_node_min_id = global_id;
	for (int i = 0; i < NUM_CONTROL_NODES; i++)
	{
		ControlNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		node.bot_head_tx = 0;
		node.bot_tail_tx = 0;
		node.bot_head_rx = 0;
		node.bot_tail_rx = 0;
		control_nodes[i] = node;
	}
	uint32_t control_node_max_id = global_id - 1;

	// switch nodes
	SwitchNode switch_nodes[NUM_SWITCH_NODES];
	uint32_t switch_mode_min_id = global_id;
	for (int i = 0; i < NUM_SWITCH_NODES; i++)
	{
		SwitchNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		node.top_head_tx = 0;
		node.top_tail_tx = 0;
		node.top_head_rx = 0;
		node.top_tail_rx = 0;
		node.bot_head_tx = 0;
		node.bot_tail_tx = 0;
		node.bot_head_rx = 0;
		node.bot_tail_rx = 0;
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
		node.top_head_tx = 0;
		node.top_tail_tx = 0;
		node.top_head_rx = 0;
		node.top_tail_rx = 0;
		memory_nodes[i] = node;
	}
	uint32_t memory_node_max_id = global_id - 1;

	// TODO TESTING REMOVE
	// create test data for packet
	DataNode data_node;
	data_node.type = WRITE;
	data_node.addr = 0x12345678;
	data_node.data = 0xDEADBEEF;
	// create packet with data
	Packet test_packet;
	test_packet.id = global_id;
	global_id++;
	test_packet.time = global_time;
	test_packet.src = control_nodes[0].id;
	test_packet.dst = memory_nodes[0].id;
	test_packet.data = data_node;
	// place a packet in control node destined to memory node
	control_nodes[0].bot_ports_tx[0][control_nodes[0].bot_tail_tx] = test_packet;
	control_nodes[0].bot_tail_tx++;

	// main loop
	for (; global_time < 10;) // TODO loop through packets to send
	{
		printf("--- GLOBAL TIME = %d\n", global_time);
		// outgoing from control nodes
		for (int i = 0; i < NUM_CONTROL_NODES; i++)
		{
			printf("* Control node with ID %d at time %d\n", control_nodes[i].id, global_time);
			// loop through outgoing ports to switch nodes
			for (int j = 0; j < CTRL_NUM_BOT_PORTS; j++)
			{
				Packet curr_packet_tx = control_nodes[i].bot_ports_tx[j][control_nodes[i].bot_head_tx];
				Packet curr_packet_rx = control_nodes[i].bot_ports_rx[j][control_nodes[i].bot_head_rx];
				if ((control_nodes[i].bot_head_tx != control_nodes[i].bot_tail_tx) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= memory_node_min_id) && (curr_packet_tx.dst <= memory_node_max_id))
					{
						printf("Moving packet with ID %d to switch with ID %d\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place node into switch
						switch_nodes[0].top_ports_rx[i][switch_nodes[0].top_tail_rx] = curr_packet_tx;
						switch_nodes[0].top_tail_rx++;
						// remove node from control node
						control_nodes[i].bot_head_tx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
						control_nodes[i].bot_head_tx++;
					}
				}

				if ((control_nodes[i].bot_head_rx != control_nodes[i].bot_tail_rx) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if (curr_packet_rx.dst == control_nodes[i].id)
					{
						printf("Packet with ID %d has arrived at control node with ID %d\n", curr_packet_tx.id, control_nodes[i].id);
						// TODO act on this
						// remove node from control node
						control_nodes[i].bot_head_rx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
						control_nodes[i].bot_head_rx++;
					}
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
				Packet curr_packet_tx = switch_nodes[i].bot_ports_tx[j][switch_nodes[i].bot_head_tx];
				Packet curr_packet_rx = switch_nodes[i].bot_ports_rx[j][switch_nodes[i].bot_head_rx];
				if ((switch_nodes[i].bot_head_tx != switch_nodes[i].bot_tail_tx) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= memory_node_min_id) && (curr_packet_tx.dst <= memory_node_max_id))
					{
						printf("Moving packet with ID %d to memory node with ID %d\n", curr_packet_tx.id, memory_nodes[i].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place node into switch
						memory_nodes[i].top_ports_rx[i][memory_nodes[i].top_tail_rx] = curr_packet_tx;
						memory_nodes[i].top_tail_rx++;
						// remove node from control node
						switch_nodes[i].bot_head_tx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
						switch_nodes[i].bot_head_tx++;
					}
				}

				if ((switch_nodes[i].bot_head_rx != switch_nodes[i].bot_tail_rx) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_rx.dst >= control_node_min_id) && (curr_packet_rx.dst <= control_node_max_id))
					{
						printf("Moving packet with ID %d to output queue\n", curr_packet_rx.id);
						curr_packet_rx.time += GLOBAL_TIME_INCR;
						// place node into switch
						switch_nodes[0].top_ports_tx[i][switch_nodes[0].top_tail_tx] = curr_packet_rx;
						switch_nodes[0].top_tail_tx++;
						// remove node from control node
						switch_nodes[0].bot_head_rx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
						switch_nodes[0].bot_head_rx++;
					}
				}
			}

			for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
			{
				Packet curr_packet_tx = switch_nodes[i].top_ports_tx[j][switch_nodes[i].top_head_tx];
				Packet curr_packet_rx = switch_nodes[i].top_ports_rx[j][switch_nodes[i].top_head_rx];
				if ((switch_nodes[i].top_head_tx != switch_nodes[i].top_tail_tx) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= control_node_min_id) && (curr_packet_tx.dst <= control_node_max_id))
					{
						printf("Moving packet with ID %d to control node with ID %d\n", curr_packet_tx.id, control_nodes[i].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place node into switch
						control_nodes[i].bot_ports_rx[i][control_nodes[i].bot_tail_rx] = curr_packet_tx;
						control_nodes[i].bot_tail_rx++;
						// remove node from control node
						switch_nodes[i].top_head_tx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
						switch_nodes[i].top_head_tx++;
					}
				}

				if ((switch_nodes[i].top_head_rx != switch_nodes[i].top_tail_rx) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_rx.dst >= memory_node_min_id) && (curr_packet_rx.dst <= memory_node_max_id))
					{
						printf("Moving packet with ID %d to output queue\n", curr_packet_rx.id);
						curr_packet_rx.time += GLOBAL_TIME_INCR;
						// place node into switch
						switch_nodes[0].bot_ports_tx[i][switch_nodes[0].bot_tail_tx] = curr_packet_rx;
						switch_nodes[0].bot_tail_tx++;
						// remove node from control node
						switch_nodes[0].top_head_rx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
						switch_nodes[0].top_head_rx++;
					}
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
				Packet curr_packet_tx = memory_nodes[i].top_ports_tx[j][memory_nodes[i].top_head_tx];
				Packet curr_packet_rx = memory_nodes[i].top_ports_rx[j][memory_nodes[i].top_head_rx];
				if ((memory_nodes[i].top_head_tx != memory_nodes[i].top_tail_tx) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= control_node_min_id) && (curr_packet_tx.dst <= control_node_max_id))
					{
						printf("Moving packet with ID %d to switch with ID %d\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time += GLOBAL_TIME_INCR;
						// place node into switch
						switch_nodes[0].bot_ports_rx[i][switch_nodes[0].bot_tail_rx] = curr_packet_tx;
						switch_nodes[0].bot_tail_rx++;
						// remove node from control node
						memory_nodes[i].top_head_tx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
						memory_nodes[i].top_head_tx++;
					}
				}

				if ((memory_nodes[i].top_head_rx != memory_nodes[i].top_tail_rx) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if (curr_packet_rx.dst == memory_nodes[i].id)
					{
						printf("Packet with ID %d has arrived at memory node with ID %d\n", curr_packet_tx.id, memory_nodes[i].id);
						// TODO act on this
						// remove node from control node
						memory_nodes[i].top_head_rx++;
					}
					else
					{
						printf("Packet with ID %d has invalid destination\n", curr_packet_rx.id);
						memory_nodes[i].top_head_rx++;
					}
				}
			}
		}
		
		// end of loop
		global_time += GLOBAL_TIME_INCR;

		// return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}