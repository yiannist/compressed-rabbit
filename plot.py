#! /usr/bin/env python

import matplotlib.pyplot as plt
import numpy as np
import sys

if len(sys.argv) < 2:
    print "./plot.py <output_file>"
    exit(1)

output_filename = sys.argv[1]
f = open(output_filename)


# Read lines from file and split them
lines = [line.strip().split() for line in f.readlines()]
msg_sizes = [int(i)/1000.0 for i in lines[2][2:] if i != 'B']

# Remove comment/size lines and split
lines = lines[3:]

runs_cmpr, runs_msgs = [], []
for l in lines:
    line = [float(i)*1000 for i in l]

    compression_latency = line[1::2]
    message_ack_latency = line[2::2]

    runs_cmpr.append(compression_latency)
    runs_msgs.append(message_ack_latency)

# Transpose magic!
msgs_latency_per_size = [list(a) for a in zip(*runs_msgs)]
cmpr_latency_per_size = [list(a) for a in zip(*runs_cmpr)]

# Compute median and stddev of different runs per message size
msgs_latency_median_per_size = [np.median(a) for a in msgs_latency_per_size]
cmpr_latency_median_per_size = [np.median(a) for a in cmpr_latency_per_size]

msgs_latency_stddev_per_size = [np.std(a) for a in msgs_latency_per_size]
cmpr_latency_stddev_per_size = [np.std(a) for a in cmpr_latency_per_size]

# Plot
plt.ylabel('Latency (in ms)')
plt.xlabel('Message size (in KB)')
plt.errorbar(msg_sizes, cmpr_latency_median_per_size,
             yerr=cmpr_latency_stddev_per_size, fmt="ko", label='Compression')
plt.errorbar(msg_sizes, msgs_latency_median_per_size,
             yerr=msgs_latency_stddev_per_size, fmt="rs", label='Enqueue message')
plt.legend()

plt.show()
#plt.savefig('latency.png')
