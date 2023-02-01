import argparse
import socket
import math


# Configurable
# Must be the same values as the server
MAX_BYTES_FOR_FILE_SIZE = 2

# Auto-generated
MAX_FILE_SIZE = ((2 ** 8) ** MAX_BYTES_FOR_FILE_SIZE) - 1


def get_args():
    """
    Get the program arguments
    """
    parser = argparse.ArgumentParser(prog='LinuxMemoryExtractor Client')
    parser.add_argument('host')
    parser.add_argument('port')
    parser.add_argument('filename')
    return parser.parse_args()


args = get_args()

file = open(args.filename, "rb")
file_data = file.read()
file.close()

file_length = len(file_data)

s = socket.socket()
s.connect((args.host, int(args.port)))

number_of_packets = math.ceil(file_length / MAX_FILE_SIZE)

for i in range(number_of_packets):
    packet_data = file_data[i * MAX_FILE_SIZE:(i+1) * MAX_FILE_SIZE]
    packet_data_length = min(MAX_FILE_SIZE, file_length - (i * MAX_FILE_SIZE))
    last_packet = (i == number_of_packets - 1)
    if last_packet:
        last_packet_flag = int(1).to_bytes(1, "little")
    else:
        last_packet_flag = int(0).to_bytes(1, "little")

    packet_message = \
        packet_data_length.to_bytes(MAX_BYTES_FOR_FILE_SIZE, "little") \
        + last_packet_flag + packet_data

    print(f"Sending packet {i+1}/{number_of_packets}"
          + f" (Length = {packet_data_length})")
    s.sendall(packet_message)

print("All packets sent successfully")
