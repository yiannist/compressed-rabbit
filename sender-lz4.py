#!/usr/bin/env python

import lz4
import pika
import sys

data = str(sys.argv[1])
print " <-- Message is: '%s' with size %d" % (data, len(data))

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.queue_declare(queue='archipelago')

cmpData = lz4.compress(data)
print " <-- Compressed raw data is: %r with size %d" % (cmpData, len(cmpData))

channel.basic_publish(exchange='',
                      routing_key='archipelago',
                      body=cmpData)
print " [x] Sent lz4ified '%s'" % data
connection.close()
