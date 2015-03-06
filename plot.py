#! /usr/bin/env python

import matplotlib.pyplot as plt
import sys

if len(sys.argv) < 2:
    print "./plot.py <output_file>"
    exit(1)

output_filename = sys.argv[1]
f = open(output_filename)

# Get rid of run # and strip()
lines = [line.strip() for line in f.readlines()]

msg_sizes = [int(i)/1000 for i in lines[2].split()[2:] if i != 'B']

# Remove comment/size lines and split
lines = [l.split() for l in lines[3:]]

line = [float(i)*1000 for i in lines[0]]

compression_latency = line[1::2]
message_ack_latency = line[2::2]

plt.ylabel('Latency (in ms)')
plt.xlabel('Message size (in KB)')
plt.plot(msg_sizes, compression_latency, "ko", label='Compression', markersize=7)
plt.plot(msg_sizes, message_ack_latency, "rs", label='Enqueue message', markersize=7)
plt.legend()

#plt.show()
plt.savefig('latency.png')
