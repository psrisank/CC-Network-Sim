#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"

#include "main.h"
#include "compute_node.h"
#include "switch_node.h"
#include "memory_node.h"
#include "packet.h"
#include "port.h"
#include <stdbool.h>

#define DEBUG

int main(int argc, char ** argv)
{
	if (argc < 4) {
		printf(ANSI_COLOR_RED "Usage: ./[exec] [packet input file] [memory input file] [packet log file]" ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}


	// Initialize global times and IDs
	uint32_t global_time = 0;
	uint32_t global_id = 0;

	#ifdef DEBUG
		//printf(ANSI_COLOR_RED     "RED"     ANSI_COLOR_RESET " ");
		//printf(ANSI_COLOR_GREEN   "GREEN"   ANSI_COLOR_RESET " ");
		//printf(ANSI_COLOR_YELLOW  "YELLOW"  ANSI_COLOR_RESET " ");
		//printf(ANSI_COLOR_BLUE    "BLUE"    ANSI_COLOR_RESET " ");
		//printf(ANSI_COLOR_MAGENTA "MAGENTA" ANSI_COLOR_RESET " ");
		//printf(ANSI_COLOR_CYAN    "CYAN"    ANSI_COLOR_RESET "\n\n");
	#endif

	//printf("Trace input file: %s\n", argv[1]);
	//printf("Memory input file: %s\n", argv[2]);
	//printf("Packet log: %s\n\n", argv[3]);

	// Initialization of all compute nodes and their caches
	ComputeNode compute_nodes[NUM_COMPUTE_NODES];
	uint32_t compute_node_min_id = global_id;
	for (int i = 0; i < NUM_COMPUTE_NODES; i++) {
		ComputeNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < CMP_NUM_BOT_PORTS; j++) {
			node.bot_ports[j].tail_tx = 0;
			node.bot_ports[j].tail_rx = 0;
		}
		for (int j = 0; j < CACHE_LINES; j++) {
			node.cache[j] = (ComputeNodeMemoryLine) {0, 0, 0, 0, 0};
		}
		updateNodeState(&node);
		compute_nodes[i] = node;
	}
	uint32_t compute_node_max_id = global_id - 1;

	// //printf("Node 1: \n");
	// //printf("Cache line 0 state: %d\n", compute_nodes[1].cache[0].state);
	// //printf("Cache line 1 state: %d\n", compute_nodes[1].cache[1].state);
	// //printf("Cache line 2 state: %d\n", compute_nodes[1].cache[2].state);
	// //printf("Cache line 3 state: %d\n", compute_nodes[1].cache[3].state);

	// Switch Node Initialization
	SwitchNode switch_nodes[NUM_SWITCH_NODES];
	//uint32_t switch_node_min_id = global_id;
	for (int i = 0; i < NUM_SWITCH_NODES; i++) {
		SwitchNode node;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < SW_NUM_TOP_PORTS; j++) {
			node.top_ports[j].tail_rx = 0;
			node.top_ports[j].tail_tx = 0;
		}
		for (int j = 0; j < SW_NUM_BOT_PORTS; j++) {
			node.bot_ports[j].tail_rx = 0;
			node.bot_ports[j].tail_tx = 0;
		}
		switch_nodes[i] = node;
	}
	//uint32_t switch_node_max_id = global_id - 1;

	// Memory Node Initialization
	// MemoryNode memory_nodes[NUM_MEMORY_NODES];
	MemoryNode* memory_nodes = malloc(sizeof(MemoryNode) * NUM_MEMORY_NODES);
	uint32_t memory_node_min_id = global_id;
	for (int i = 0; i < NUM_MEMORY_NODES; i++) {
		MemoryNode node;
		node.id = global_id;
		global_id++;
		node.time = global_time;
		for (int j = 0; j < MEM_NUM_TOP_PORTS; j++) {
			node.top_ports[j].tail_rx = 0;
			node.top_ports[j].tail_tx = 0;
		}
		memory_nodes[i] = node;
	}
	//uint32_t memory_node_max_id = global_id - 1;
	// for (int i = 0; i < NUM_MEMORY_NODES; i++) {
	// 	init_memnodes(&(memory_nodes[i]), NUM_COMPUTE_NODES);
	// }

	// for (int i = 0; i < NUM_MEMORY_NODES; i++) {
	// 	printf("%d\n", memory_nodes[i].memory[0].nodeState[0]);
	// }


	// Memory initialization
 	char line[255];
	char* token;
	FILE* mem_input;
	mem_input = fopen(argv[2], "r");
	if (mem_input == NULL) {
		//printf(ANSI_COLOR_RED "Input file does not exist!" ANSI_COLOR_RESET "\n");
		return EXIT_FAILURE;
	}
	
	while (fgets(line, 255, mem_input) != NULL) {
		MemoryLine data_line;
		int curr_field = 0;
		token = strtok(line, ",");
		while (token != NULL) {
			switch(curr_field) {
				// address
				case 0: {
					data_line.address = (uint32_t) strtol(token, NULL, 16);
					break;
				}

				// data value
				case 1: {
					data_line.value = (uint32_t) strtol(token, NULL, 16);
				}
				
				// Future work: have memory keep track of the state of the address in each cache
			}
			token = strtok(NULL, ",");
			curr_field++;
		}
		uint8_t memory_node_num = ((data_line.address << 8) / (MEM_NUM_LINES * MEM_LINE_SIZE)) / MEM_NUM_LINES;
 		uint8_t memory_node_line = ((data_line.address << 8) / (MEM_NUM_LINES * MEM_LINE_SIZE)) % MEM_NUM_LINES;
		memory_nodes[memory_node_num].memory[memory_node_line] = data_line;
	}
	fclose(mem_input);

	// Creating packets based off input trace
	Packet packets[2000];
	// Packet* packets = malloc(sizeof(Packet) * 2000);
	FILE* cmd_inputs = fopen(argv[1], "r");
	if (cmd_inputs == NULL) {
		return EXIT_FAILURE;
	}
	int pkt_iterator;
	while (fgets(line, sizeof(line), cmd_inputs)) {
		uint8_t pkt_src;
		uint32_t pkt_time, pkt_addr, pkt_data;
		flag_t pkt_flag;
		token = strtok(line, ",");
		pkt_time = (uint32_t) strtol(token, NULL, 10);
		token = strtok(NULL, ",");
		pkt_src = (uint8_t) strtol(token, NULL, 10);
		token = strtok(NULL, ",");
		pkt_addr = (uint32_t) strtol(token, NULL, 16);
		token = strtok(NULL, ",");
		pkt_flag = strtol(token, NULL, 2) ? INST_WRITE : INST_READ;
		token = strtok(NULL, ",");
		pkt_data = (uint32_t) strtol(token, NULL, 16);
		packets[pkt_iterator++] = (Packet) {global_id++, pkt_time, pkt_flag, pkt_src, pkt_addr / (64 * 4), (DataNode) {pkt_addr, pkt_data}};
	}
	int pkt_cnt = pkt_iterator;
	// //printf("PACKET CNT: %d\n", pkt_cnt);

	//Create an output file
	FILE* output_file;
	output_file = fopen(argv[3], "w");

	//printf("\nCOMPUTE NODES ID: [%d-%d], SWITCH NODES ID: [%d-%d], MEMORY NODES ID: [%d-%d]\n", compute_node_min_id, compute_node_max_id, switch_node_min_id, switch_node_max_id, memory_node_min_id, memory_node_max_id);

	// Main execution loop
	// Excecution is as follows:
	// Timer loop runs, and within the timer loop exists a file reading loop that goes through the file and grabs all the instructions
	// Then, it executes that instruction
	pkt_iterator = 0;
	int stall = 0;
	//int last_stall = 0;
	char finished = 0;
	int cooldown = 3;
	do {
		//printf("\n--- GLOBAL TIME = %d\n", global_time);

		// coherence logic
		// printf("\n\n\n\nGlobal time: %d, pkt_iterator: %d, pkt_iterator instruction time: %d, stalling: %d\n", global_time, pkt_iterator, packets[pkt_iterator].time, stall);
		if (global_time >= packets[pkt_iterator].time && pkt_iterator < pkt_cnt && !stall) {
			int state_action = check_state(compute_nodes[packets[pkt_iterator].src - compute_node_min_id], packets[pkt_iterator].data.addr);
			if (packets[pkt_iterator].flag == INST_READ) {
				switch(state_action) {
					case 1:
						packets[pkt_iterator].flag = READ;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						packets[pkt_iterator].flag = INST_READ;
						// pkt_iterator++;
						log_cdatareq();
						// stall = 1;
						break;
					case 5:
						packets[pkt_iterator].flag = READ;
						// printf("Address did not match, but in shared.\n");
						// printf("Requesting address 0x%lx.\n", packets[pkt_iterator].data.addr);
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						packets[pkt_iterator].flag = INST_READ;
						pkt_iterator++;
						log_cdatareq();
						// stall = 1;
						break;
					case 6: // request data
						packets[pkt_iterator].flag = READ;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						packets[pkt_iterator].flag = INST_READ;
						// pkt_iterator++;
						log_cdatareq();
						// stall = 1;
						break;
					case 4: // writeback data
						Packet dat_req_pkt;
						dat_req_pkt.id = global_id++;
						dat_req_pkt.time = global_time;
						dat_req_pkt.flag = WR_DATA;
						dat_req_pkt.src = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].id;
						dat_req_pkt.dst = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].address;
						dat_req_pkt.data.addr = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].address;
						dat_req_pkt.data.data = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].value;
						compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].state = SHARED;
						compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].dirty = 0;
						// compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].address = packets[pkt_iterator].data.addr;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, dat_req_pkt);
						// stall = 1;
						log_cwritedata();
						// printf("Returning data for a read.\n");
						break;
					default: pkt_iterator++; // do nothing since up to date data is in the cache
						break;
				}
			}
			else if (packets[pkt_iterator].flag == INST_WRITE) {
				// printf("Write detected! for packet time %d.\n", packets[pkt_iterator].time);
				// printf("Action is: %d.\n", state_action);
				switch(state_action) {
					case 1: // send write req
						write_action(&compute_nodes[packets[pkt_iterator].src - compute_node_min_id], packets[pkt_iterator].data.addr, packets[pkt_iterator].data.data); 
						packets[pkt_iterator].flag = WR_SIGNAL;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						pkt_iterator++;
						log_cwritereq();
						break;
					case 2: // update the cache but no need to invalidate other caches
						write_action(&compute_nodes[packets[pkt_iterator].src - compute_node_min_id], packets[pkt_iterator].data.addr, packets[pkt_iterator].data.data); 
						pkt_iterator++;
						break;
					case 3: // send write req
						write_action(&compute_nodes[packets[pkt_iterator].src - compute_node_min_id], packets[pkt_iterator].data.addr, packets[pkt_iterator].data.data); 
						packets[pkt_iterator].flag = WR_SIGNAL;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						pkt_iterator++;
						log_cwritereq();
						break;
					case 4: // writeback
						// printf("Writing back for packet time %d.\n", packets[pkt_iterator].time);
						Packet dat_req_pkt;
						dat_req_pkt.id = global_id++;
						dat_req_pkt.time = global_time;
						dat_req_pkt.flag = WR_DATA;
						dat_req_pkt.src = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].id;
						dat_req_pkt.dst = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].address;
						dat_req_pkt.data.addr = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].address;
						dat_req_pkt.data.data = compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].value;
						compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].state = SHARED;
						compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].dirty = 0;
						compute_nodes[packets[pkt_iterator].src - compute_node_min_id].cache[(packets[pkt_iterator].data.addr >> 2) % 4].address = packets[pkt_iterator].data.addr;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, dat_req_pkt);
						log_cwritedata();
						break;
					case 5: // send write req
						write_action(&compute_nodes[packets[pkt_iterator].src - compute_node_min_id], packets[pkt_iterator].data.addr, packets[pkt_iterator].data.data); 
						packets[pkt_iterator].flag = WR_SIGNAL;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						pkt_iterator++;
						log_cwritereq();
						break;
					case 6: // send write req
						write_action(&compute_nodes[packets[pkt_iterator].src - compute_node_min_id], packets[pkt_iterator].data.addr, packets[pkt_iterator].data.data); 
						packets[pkt_iterator].flag = WR_SIGNAL;
						push_packet((&(compute_nodes[packets[pkt_iterator].src - compute_node_min_id].bot_ports[0])), TX, packets[pkt_iterator]);
						pkt_iterator++;
						log_cwritereq();
						break;
					default: 
						pkt_iterator++;
						break;
				}
			}
		}

		// incoming/outgoing from compute 
		for (int i = 0; i < NUM_COMPUTE_NODES; i++)
		{
			//printf("* Compute node with ID %d at time %d\n", compute_nodes[i].id, global_time);
			// loop through outgoing ports to switch nodes
			for (int j = 0; j < CMP_NUM_BOT_PORTS; j++)
			{
				Packet curr_packet_tx = pop_packet((&(compute_nodes[i].bot_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(compute_nodes[i].bot_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					//printf(ANSI_COLOR_BLUE "Moving packet with ID %d to switch with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, switch_nodes[0].id);
					curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom value
					// place packet into switch port corresponding to compute node ID
					push_packet((&(switch_nodes[0].top_ports[i])), RX, curr_packet_tx);
					// remove packet from compute node
					pop_packet((&(compute_nodes[i].bot_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if (curr_packet_rx.dst == compute_nodes[i].id)
					{
						//printf(ANSI_COLOR_GREEN "Packet with ID %d has arrived at compute node with ID %d [0x%08x, 0x%08x]" ANSI_COLOR_RESET "\n", curr_packet_rx.id, compute_nodes[i].id, curr_packet_rx.data.addr, curr_packet_rx.data.data);
						// TODO act on this
						Packet ret_pkt = cnode_process_packet(&compute_nodes[i], curr_packet_rx, &stall);
						if (ret_pkt.flag == RESEND) {
							pkt_iterator--;
						}
						if (ret_pkt.flag != ERROR) {
							// printf("Data is being returned.\n");
							push_packet((&(switch_nodes[0].top_ports[i])), RX, ret_pkt);
						}

						//printf("Stall is now: %d\n", stall);
						//printf("Validity of index in node %d cache: %d\n", i, compute_nodes[i].cache[(curr_packet_rx.data.addr >> 2) % 4].valid);				
					}
					// remove packet from compute node
					pop_packet((&(compute_nodes[i].bot_ports[j])), RX, 1);
				}
			}
		}


		// incoming/outgoing from switch nodes
		for (int i = 0; i < NUM_SWITCH_NODES; i++)
		{
			// loop through outgoing ports to memory nodes
			//printf("* Switch node with ID %d at time %d\n", switch_nodes[i].id, global_time);

			for (int j = 0; j < SW_NUM_TOP_PORTS; j++)
			{
				//printf("** Top port %d\n", j);
				Packet curr_packet_tx = pop_packet((&(switch_nodes[i].top_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(switch_nodes[i].top_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_tx.dst >= compute_node_min_id) && (curr_packet_tx.dst <= compute_node_max_id))
					{
						//printf(ANSI_COLOR_BLUE "Moving packet with ID %d to compute node with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, curr_packet_tx.dst);
						curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into compute node
						push_packet((&(compute_nodes[j].bot_ports[i])), RX, curr_packet_tx);
						// write to log file
						char hex_addr[11];
						char hex_data[11];
						snprintf(hex_addr, sizeof(hex_addr), "0x%08X", curr_packet_tx.data.addr);
						snprintf(hex_data, sizeof(hex_data), "0x%08X", curr_packet_tx.data.data);
						fprintf(output_file, "%d,%d,%d,%d,%d,%s,%s\n", global_time, curr_packet_tx.id, curr_packet_tx.flag, curr_packet_tx.src, curr_packet_tx.dst, hex_addr, hex_data);
					}
					else
					{
						//printf("Packet with ID %d has invalid destination\n", curr_packet_tx.id);
					}
					// remove packet from switch top output port
					pop_packet((&(switch_nodes[i].top_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// printf("Sending data back to memory node.\n");
					//printf(ANSI_COLOR_BLUE "Moving packet with ID %d to output queue" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
					curr_packet_rx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
					// place node into switch bottom output port corresponding to address
					//printf("Data address: %x\n", curr_packet_rx.data.addr);
					//uint32_t correct_memory_node = (curr_packet_rx.data.addr >> 4) / 4096;//curr_packet_rx.data.addr / 4096;//((curr_packet_rx.data.addr >> 4) / 64);
					// uint32_t correct_memory_node = (curr_packet_rx.data.addr >> 2) & 0xF;	
					uint32_t correct_memory_node = curr_packet_rx.data.addr / (64 * 4);
					// printf("Correct memory node for address: 0x%lx is node index: %d\n", curr_packet_rx.data.addr, correct_memory_node);
					curr_packet_rx.dst = correct_memory_node + memory_node_min_id;
					push_packet((&(switch_nodes[i].bot_ports[correct_memory_node])), TX, curr_packet_rx);
					// remove packet from switch top input port
					pop_packet((&(switch_nodes[i].top_ports[j])), RX, 1);
				}
			}

			for (int j = 0; j < SW_NUM_BOT_PORTS; j++)
			{
				//printf("** Bottom port %d\n", j);
				Packet curr_packet_tx = pop_packet((&(switch_nodes[i].bot_ports[j])), TX, 0);
				Packet curr_packet_rx = pop_packet((&(switch_nodes[i].bot_ports[j])), RX, 0);
				if ((curr_packet_tx.flag != ERROR) && (curr_packet_tx.time <= global_time))
				{
					//printf(ANSI_COLOR_BLUE "Moving packet with ID %d to memory node with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, curr_packet_tx.dst);
					curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
					// place packet into memory node
					push_packet((&(memory_nodes[j].top_ports[i])), RX, curr_packet_tx);
					// write to log file
					char hex_addr[11];
					char hex_data[11];
					snprintf(hex_addr, sizeof(hex_addr), "0x%08X", curr_packet_tx.data.addr);
					snprintf(hex_data, sizeof(hex_data), "0x%08X", curr_packet_tx.data.data);
					fprintf(output_file, "%d,%d,%d,%d,%d,%s,%s\n", global_time, curr_packet_tx.id, curr_packet_tx.flag, curr_packet_tx.src, curr_packet_tx.dst, hex_addr, hex_data);
					// remove packet from switch bottom output port
					pop_packet((&(switch_nodes[i].bot_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					if ((curr_packet_rx.dst >= compute_node_min_id) && (curr_packet_rx.dst <= compute_node_max_id))
					{
						//printf(ANSI_COLOR_BLUE "Moving packet with ID %d to output queue" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
						curr_packet_rx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into switch top output port
						uint32_t correct_compute_node = curr_packet_rx.dst - compute_node_min_id;
						push_packet((&(switch_nodes[i].top_ports[correct_compute_node])), TX, curr_packet_rx);
					}
					else
					{
						//printf(ANSI_COLOR_RED "Packet with ID %d has invalid destination" ANSI_COLOR_RESET "\n", curr_packet_rx.id);
					}
					// remove packet from switch bottom input port
					pop_packet((&(switch_nodes[i].bot_ports[j])), RX, 1);
				}
			}
		}

		// incoming/outgoing from memory nodes
		for (int i = 0; i < NUM_MEMORY_NODES; i++)
		{
			//printf("* Memory node with ID %d at time %d\n", memory_nodes[i].id, global_time);
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
						//printf(ANSI_COLOR_BLUE "Moving packet with ID %d to switch with ID %d" ANSI_COLOR_RESET "\n", curr_packet_tx.id, switch_nodes[0].id);
						curr_packet_tx.time = global_time + GLOBAL_TIME_INCR; // TODO custom time
						// place packet into switch
						push_packet((&(switch_nodes[0].bot_ports[i])), RX, curr_packet_tx);
					}
					else
					{
						//printf(ANSI_COLOR_RED "Packet with ID %d has invalid destination" ANSI_COLOR_RESET "\n", curr_packet_tx.id);
					}
					// remove packet from memory node
					pop_packet((&(memory_nodes[i].top_ports[j])), TX, 1);
				}

				if ((curr_packet_rx.flag != ERROR) && (curr_packet_rx.time <= global_time))
				{
					// need to act on this packet
					//printf(ANSI_COLOR_GREEN "Packet with ID %d has arrived at memory node with ID %d [0x%08x, 0x%08x]" ANSI_COLOR_RESET "\n", curr_packet_rx.id, memory_nodes[i].id, curr_packet_rx.data.addr, curr_packet_rx.data.data);
					// handle return packet
					Packet return_packet = process_packet(&memory_nodes[i], curr_packet_rx, global_id++, global_time, memory_node_min_id, &(memory_nodes[i].top_ports[j]));
					if (return_packet.flag != ERROR)
					{
						// place packet in outgoing buffer
						push_packet((&(memory_nodes[i].top_ports[j])), TX, return_packet);
					}
					// remove packet from memory node
					pop_packet((&(memory_nodes[i].top_ports[j])), RX, 1);
				}
			}
		}








		global_time++;
		if ((packets[pkt_cnt - 1].time + 200) == global_time) {
			finished = 1;
		}
	} while (finished == 0);

	printf("Ended on global time: %d\n", global_time);
    for (uint32_t i = 0; i <= compute_node_max_id; i++) {
        // printf("Compute node %d\n----------------\n", i);
        // print_cache(&compute_nodes[i]);
        // printf("\n\n");
    }

	get_compute_control_count();
	printf("Memory node to control node with data :%d\n\n", get_memory_control_count());


	fclose(output_file);
	printf("\n");
	return EXIT_SUCCESS;
}