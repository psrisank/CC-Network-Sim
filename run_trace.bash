#!/bin/bash




cd ../traces
python3 generateTrace.py "cluster$1.sort"
cd ../CC-Network-Sim
mv ../traces/input_trace.csv artifacts/input_trace.csv
mv ../traces/meminit.csv artifacts/meminit.csv

# Run simulator for the RDMA Multicast test case
sed -i '6s/^#define MAX_PKT_SIZE.*/#define MAX_PKT_SIZE 72 \/\/ RDMA: 72 bytes | EDC: 8 bytes/' src/packet.h
sed -i '7s/^#define MULTICAST.*/#define MULTICAST 1 \/\/ Whether to multicast or not/' src/packet.h
echo "cluster$1 RDMA Multicast"
make > tmp.log
python3 generate_graphs.py "RDMA Multicast" $1 tmp.log

# Run simulator for RDMA Unicast test case
sed -i '6s/^#define MAX_PKT_SIZE.*/#define MAX_PKT_SIZE 72 \/\/ RDMA: 72 bytes | EDC: 8 bytes/' src/packet.h
sed -i '7s/^#define MULTICAST.*/#define MULTICAST 0 \/\/ Whether to multicast or not/' src/packet.h
echo "cluster$1 RDMA Unicast"
make > tmp.log
python3 generate_graphs.py "RDMA Unicast" $1 tmp.log


# Run simulator for EDC Multicast test case
sed -i '6s/^#define MAX_PKT_SIZE.*/#define MAX_PKT_SIZE 8 \/\/ RDMA: 72 bytes | EDC: 8 bytes/' src/packet.h
sed -i '7s/^#define MULTICAST.*/#define MULTICAST 1 \/\/ Whether to multicast or not/' src/packet.h
echo "cluster$1 EDC Multicast"
make > tmp.log
python3 generate_graphs.py "EDC Multicast" $1 tmp.log

# Run simulator for EDC Multicast test case
sed -i '6s/^#define MAX_PKT_SIZE.*/#define MAX_PKT_SIZE 8 \/\/ RDMA: 72 bytes | EDC: 8 bytes/' src/packet.h
sed -i '7s/^#define MULTICAST.*/#define MULTICAST 0 \/\/ Whether to multicast or not/' src/packet.h
echo "cluster$1 EDC Unicast"
make > tmp.log
python3 generate_graphs.py "EDC Unicast" $1 tmp.log


