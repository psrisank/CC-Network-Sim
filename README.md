# CC-Network-Sim
Network simulator implemented in C with cache coherence protocol

## Goals

- [ ] Send packets from control node to memory node
- [ ] Send packets from memory node to control node (concurrently)
- [ ] Generate packets from desired parameters (to be executed by C simulator)
- [ ] Cache coherence over simulator
- [ ] Save packet information to file

## Logic

The general premise of the simulator consists of control nodes, switch nodes, and memory nodes.  The packets to be sent are generated and placed into TX queues of the control nodes with destinations to the memory nodes.  In absolute time, the packets are sent to switch queues, and then the memory nodes.  The reverse is also possible.

The simulator aims to simulate full-duplex connections, with independent TX and RX queues for each node.  The time it takes for packets to be processed and transmitted should also be parameterized.