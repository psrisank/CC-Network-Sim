#ifndef __MEMORY_NODE_H__
#define __MEMORY_NODE_H__

#include "stdint.h"
#include "port.h"
#include "packet.h"
#include "stdlib.h"
#include "compute_node.h"

// definitions for memory node
#define MEM_NUM_TOP_PORTS	1	// should not ever be changed
#define MEM_QUEUE_SIZE		256	// in packets
#define MEM_NUM_LINES		20000 // 8192 originally for 64 addresses 128 nodes
#define MEM_LINE_SIZE		32	// in bits

// enum defining various cache states for MSI protocol
typedef enum
{
	I, S, M
}
StatesMSI_t;

// struct holding data inside a MemoryNode
typedef struct MemoryLine
{
	uint64_t address;
	uint32_t value;
	// status of all compute nodes
	// StatesMSI_t* nodeState;
	state_t nodeState[128];
}
MemoryLine;

// top-level struct for a memory node
typedef struct MemoryNode
{
	uint8_t id;
	uint32_t time;

	Port top_ports[MEM_NUM_TOP_PORTS];

	MemoryLine memory[MEM_NUM_LINES];
}
MemoryNode;

void init_memnodes(MemoryNode* node, int node_cnt);
Packet process_packet(MemoryNode* node, Packet pkt, uint32_t global_id, uint32_t global_time);
void generate_invalidations(MemoryNode* node, Packet pkt, Port* p, uint32_t global_id, uint32_t global_time);
long get_memory_control_count();
long get_memory_to_compute_requests();
long transfer_requests();
long get_memory_to_switch_invalidations();

#endif