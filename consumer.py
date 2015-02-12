#!/usr/bin/python

import lz4
import pika

connection = pika.BlockingConnection(pika.ConnectionParameters(host='localhost'))
channel = connection.channel()

channel.queue_declare(queue='archipelago')

print " [*] Waiting for messages. To exit press CTRL+C"

def callback(ch, method, properties, cbody):
    print " --> Received raw data: %r with size %d" % (cbody, len(cbody))
    print " --> Trying to uncompress..."
    body = lz4.uncompress(cbody)
    print " [x] Received '%s' with size %d" % (body, len(body))

channel.basic_consume(callback, queue='archipelago', no_ack=True)

channel.start_consuming()
