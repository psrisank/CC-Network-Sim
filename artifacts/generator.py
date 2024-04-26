import sys
import random

# put variables here for trace generation
total_num_requests = 1000
time_between_packets = 10

memory_min_addr = 0x00000000
memory_max_addr = 0x00008000

if (len(sys.argv) != 5):
	print("Correct usage is 'python generator.py [write ratio] [sharing ratio] [num compute nodes] [output file name]'")
	exit()

# test parameters
write_percentage = float(sys.argv[1])
sharing_ratio = float(sys.argv[2])
num_compute_nodes = int(sys.argv[3])

# calculated values (do not edit)
memory_min_addr_int = memory_min_addr // 4
memory_max_addr_int = memory_max_addr // 4

packets = list()

# zeroth iteration of packets (prep sharing ratio)
for i in range(num_compute_nodes):
	global_time = (i * time_between_packets)
	packets.append([global_time, i, 0x00000000, 0, 0x00000000])

# first iteration of packets (random address/wdata)
for curr_packet in range(num_compute_nodes, total_num_requests + num_compute_nodes):
	global_time = (curr_packet * time_between_packets)
	compute_node_id = random.randint(0, num_compute_nodes - 1)
	address = 0
	while ((address >> 2) % 4 == 0):
		# get address that isnt going to be in first cache line
		address = random.randint(memory_min_addr_int + 1, memory_max_addr_int) * 4
	write = 0
	wdata = random.randint(0, 0xFFFFFFFF)

	packets.append([global_time, compute_node_id, address, write, wdata])

num_write = 0
num_shared = 0

# second iteration of packets (correct r/w ratio)
while (num_write < (total_num_requests * write_percentage)):
	rand_num = random.randint(num_compute_nodes, total_num_requests + num_compute_nodes - 1)
	if (packets[rand_num][3] == 0):
		# convert read to write
		packets[rand_num][3] = 1
		num_write += 1

# third iteration of packets (correct sharing ratio)
while (num_shared < (total_num_requests * sharing_ratio)):
	rand_num = random.randint(num_compute_nodes, total_num_requests + num_compute_nodes - 1)
	if (packets[rand_num][2] != 0x00000000):
		# convert to shared address
		packets[rand_num][2] = 0x00000000
		num_shared += 1

# write packet list to output file
with open(sys.argv[4], 'w') as output_file:
	for packet in packets:
		packet_formatted = f"{packet[0]},{packet[1]},0x{packet[2]:08X},{packet[3]},0x{packet[4]:08X}\n"
		output_file.write(packet_formatted)