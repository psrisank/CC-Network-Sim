import random

# put variables here for trace generation
total_num_requests = 1000
time_between_packets = 100

num_compute_nodes = 2
memory_min_addr = 0x00000000
memory_max_addr = 0x00001000

# type of test to run
# 0 = r/w percentage
# 1 = sharing ratio
test_type = 1
read_percentage = 0.5
sharing_ratio = 0.5

# calculated values (do not edit)
num_read_packets = read_percentage * total_num_requests
memory_min_addr_int = memory_min_addr // 4
memory_max_addr_int = memory_max_addr // 4

if (test_type == 1):
	total_num_requests -= int(total_num_requests * sharing_ratio)

with open("output_trace.csv", 'w') as output_file:
	for curr_packet in range(total_num_requests):
		if (test_type == 0):
			# r/w percentage
			global_time = (curr_packet * time_between_packets)
			compute_node_id = random.randint(0, num_compute_nodes - 1)
			address = random.randint(memory_min_addr_int, memory_max_addr_int) * 4
			write = 0 if (curr_packet < num_read_packets) else 1
			wdata = random.randint(0, 0xFFFFFFFF)

			packet_formatted = f"{global_time},{compute_node_id},0x{address:08X},{write},0x{wdata:08X}\n"
			output_file.write(packet_formatted)

		elif (test_type == 1):
			# sharing ratio
			global_time = (curr_packet * time_between_packets)
			compute_node_id = random.randint(0, num_compute_nodes - 1)
			address = random.randint(memory_min_addr_int + num_compute_nodes, memory_max_addr_int) * 4
			write = random.randint(0, 1)
			wdata = random.randint(0, 0xFFFFFFFF)

			packet_formatted = f"{global_time},{compute_node_id},0x{address:08X},{write},0x{wdata:08X}\n"
			output_file.write(packet_formatted)