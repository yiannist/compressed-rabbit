#!/usr/bin/python

import lz4
import pika

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.queue_declare(queue='archipelago')

print " [*] Waiting for messages. To exit press CTRL+C"

def callback(ch, method, properties, cbody):
    body = lz4.uncompress(cbody)
    print " [x] Received %r" % body

channel.basic_consume(callback, queue='archipelago', no_ack=True)

channel.start_consuming()
