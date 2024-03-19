# CC-Network-Sim
Network simulator implemented in C with cache coherence protocol

## Goals

- [X] Send packets from compute node to memory node
- [ ] Send packets from memory node to compute node (concurrently)
- [ ] Generate packets from desired parameters (to be executed by C simulator)
- [ ] Cache coherence over simulator
- [ ] Save packet information to file

## Logic

The general premise of the simulator consists of compute nodes, switch nodes, and memory nodes.  The packets to be sent are generated and placed into TX queues of the compute nodes with destinations to the memory nodes.  In absolute time, the packets are sent to switch queues, and then the memory nodes.  The reverse is also possible.

The simulator aims to simulate full-duplex connections, with independent TX and RX queues for each node.  The time it takes for packets to be processed and transmitted should also be parameterized.

## Current Progress

Currently, a test packet is created and placed into the compute node's TX queue.  In the main loop, at global time 0, the packet is sent to the switch node's input RX queue.  At global time 1, the switch processes this packet, verifies that it is destined to a memory node, and places it in it's TX queue.  At global time 2, the packet is sent to the memory node's input RX queue.  At global time 3, the packet is processed by the memory node and has "arrived."

Next steps are to verify sending packets both directions at the same time, and add logic to handle sending to different memory nodes/compute nodes through a single switch (different ports).