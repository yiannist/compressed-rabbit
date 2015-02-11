#!/usr/bin/env python
import lz4
import pika

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.queue_declare(queue='archipelago')

channel.basic_publish(exchange='',
                      routing_key='archipelago',
                      body=lz4.compress('Hello rabbit!'))
print " [x] Sent lz4ified 'Hello rabbit!'"
connection.close()
