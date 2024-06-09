#ifndef __PORT_H__
#define __PORT_H__

#include "stdint.h"

#include "packet.h"

#define QUEUE_SIZE 256


typedef enum
{
	TX, RX
}
BufferType_t;

typedef struct Port
{
	Packet tx[QUEUE_SIZE];
	uint32_t tail_tx;

	Packet rx[QUEUE_SIZE];
	uint32_t tail_rx;
}
Port;

Packet pop_packet(Port * port, BufferType_t type, char remove);
int push_packet(Port * port, BufferType_t type, Packet pkt);
char port_empty(Port port);
void print_port(Port port, BufferType_t type);

#endif