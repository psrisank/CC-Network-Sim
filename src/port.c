#include "stdio.h"
#include "string.h"

#include "port.h"

Packet pop_packet(Port * port, BufferType_t type, char remove)
{
	Packet err_packet;
	err_packet.flag = ERROR;

	if (type == TX)
	{
		// pop from TX buffer
		if ((* port).tail_tx != 0)
		{
			Packet pkt = (* port).tx[0];
			if (remove)
			{
				memmove((* port).tx, (* port).tx + sizeof(Packet), QUEUE_SIZE);
				(* port).tail_tx--;
			}
			return pkt;
		}
		else
		{
			return err_packet;
		}
	}
	else if (type == RX)
	{
		// pop from RX buffer
		if ((* port).tail_rx != 0)
		{
			Packet pkt = (* port).rx[0];
			if (remove)
			{
				memmove((* port).rx, (* port).rx + sizeof(Packet), QUEUE_SIZE);
				(* port).tail_rx--;
			}
			return pkt;
		}
		else
		{
			return err_packet;
		}
	}
	else
	{
		return err_packet;
	}
}

int push_packet(Port * port, BufferType_t type, Packet pkt)
{
	if (type == TX)
	{
		if ((* port).tail_tx >= (QUEUE_SIZE - 1))
		{
			// no room in buffer
			return -1;
		}
		else
		{
			// room in buffer
			(* port).tx[(* port).tail_tx] = pkt;
			(* port).tail_tx++;
			return 0;
		}
	}
	else if (type == RX)
	{
		if ((* port).tail_rx >= (QUEUE_SIZE - 1))
		{
			// no room in buffer
			return -1;
		}
		else
		{
			// room in buffer
			(* port).rx[(* port).tail_rx] = pkt;
			(* port).tail_rx++;
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

void print_port(Port port, BufferType_t type)
{
	if (type == TX)
	{
		if (port.tail_tx == 0)
		{
			printf("TX port is empty!\n");
		}
		else
		{
			for (uint32_t i = 0; i < port.tail_tx; i++)
			{
				printf("TX port index %d contains packet ID %d\n", i, port.tx[i].id);
			}
		}
	}
	else if (type == RX)
	{
		if (port.tail_rx == 0)
		{
			printf("RX port is empty!\n");
		}
		else
		{
			for (uint32_t i = 0; i < port.tail_rx; i++)
			{
				printf("RX port index %d contains packet ID %d\n", i, port.rx[i].id);
			}
		}
	}
	else
	{
		printf("Invalid port type!\n");
	}
}