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

	// control nodes
	ControlNode control_nodes[NUM_CONTROL_NODES];
	for (int i = 0; i < NUM_CONTROL_NODES; i++)
	{
		ControlNode node;
		node.id = i;
		node.time = global_time;
		control_nodes[i] = node;
	}

	// switch nodes
	SwitchNode switch_nodes[NUM_SWITCH_NODES];
	for (int i = 0; i < NUM_SWITCH_NODES; i++)
	{
		SwitchNode node;
		node.id = i;
		node.time = global_time;
		node.top_head = 0;
		node.bot_head = 0;
		switch_nodes[i] = node;
	}

	// memory nodes
	MemoryNode memory_nodes[NUM_MEMORY_NODES];
	for (int i = 0; i < NUM_MEMORY_NODES; i++)
	{
		MemoryNode node;
		node.id = i;
		node.time = global_time;
		memory_nodes[i] = node;
	}

	// main loop
	for (;;) // TODO loop through packets to send
	{
		// outgoing from control nodes
		for (int i = 0; i < NUM_CONTROL_NODES; i++)
		{
			printf("Control node with id %d at time %d\n", control_nodes[i].id, control_nodes[i].time);
		}

		// outgoing from switch nodes
		for (int i = 0; i < NUM_SWITCH_NODES; i++)
		{
			// TODO handle both top and bottom queues
			printf("Switch node with id %d at time %d\n", switch_nodes[i].id, switch_nodes[i].time);
		}

		// // outgoing from memory nodes
		for (int i = 0; i < NUM_MEMORY_NODES; i++)
		{
			printf("Memory node with id %d at time %d\n", memory_nodes[i].id, memory_nodes[i].time);
		}
		
		// end of loop
		global_time += GLOBAL_TIME_INCR;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}