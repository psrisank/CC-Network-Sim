import sys
import random
import math

# variables for trace generation
total_num_requests = 1000
time_between_packets = 200

memory_min_addr = 0x00000000
memory_max_addr = 0x00008000
# memory_max_addr = 0x00080000
# memory_max_addr = 0x0
# memory_max_addr = 0x7E00
if (len(sys.argv) != 5):
	print("Correct usage is 'python generator.py [write ratio] [sharing ratio] [num compute nodes] [output file name]'")
	exit()

# test parameters
write_percentage = float(sys.argv[1])
print(write_percentage)
sharing_ratio = float(sys.argv[2])
print(sharing_ratio)
num_compute_nodes = int(sys.argv[3])

#print("Sharing ratio: " + sharing_ratio + " Write percentage: " + write_percentage)

# calculated values
memory_min_addr_int = memory_min_addr // 4
memory_max_addr_int = memory_max_addr // 4

init_packets = list()
packets = list()

# prepare sharing ratio
for i in range(num_compute_nodes):
    global_time = (i * time_between_packets)
    init_packets.append([global_time, i, 0x00000000, 0, 0x00000000])


# create a bunch of reads from random addresses
for curr_packet in range(num_compute_nodes, total_num_requests + num_compute_nodes):
    global_time = 0
    compute_node_id = random.randint(0, num_compute_nodes - 1)
    address = 0
    #while ((address >> 2) % 4 == 0):
    while (address == 0):
        address = random.randint(memory_min_addr, memory_max_addr)
        address = address & (0xFFFFFF00)
    write = 0
    wdata = random.randint(0, 0xFFFFFFFF)
    packets.append([global_time, compute_node_id, address, write, wdata])

num_write = 0
num_shared = 0

# resolve sharing ratio
sharing_list = list()
for i in range(0, math.floor(total_num_requests * sharing_ratio)):
    entry = random.randint(0, total_num_requests - 1)
    while entry in sharing_list:
        entry = random.randint(0, total_num_requests - 1)
    sharing_list.append(entry)
print(len(sharing_list))

for i in range(len(sharing_list)):
    packets[sharing_list[i]][2] = 0x00000000

# resolve read/write ratio
write_list = list()
print(math.floor(total_num_requests * write_percentage))
for i in range(0, math.floor(total_num_requests * write_percentage)):
    entry = random.randint(0, total_num_requests - 1)
    while entry in write_list:
        entry = random.randint(0, total_num_requests - 1)
    write_list.append(entry)

for i in range(len(write_list)):
    packets[write_list[i]][3] = 1

# shuffle packets
for i in range(128):
	random.shuffle(packets)

# assign global times to packets
for i in range(total_num_requests):
    packets[i][0] = (i * time_between_packets) + (num_compute_nodes * time_between_packets)


print(len(packets))

with open(sys.argv[4], 'w') as output_file:
	for packet in init_packets:
		packet_formatted = f"{packet[0]},{packet[1]},{packet[2]:08X},{packet[3]},0x{packet[4]:08X}\n"
		output_file.write(packet_formatted)
	for packet in packets:
		packet_formatted = f"{packet[0]},{packet[1]},{packet[2]:08X},{packet[3]},0x{packet[4]:08X}\n"
		output_file.write(packet_formatted)