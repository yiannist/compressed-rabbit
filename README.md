This repo is a playground for AMQP queues and LZ4 compression in C/Python.

# Usage

  Usage: ./consumer <queuename>
  Usage: ./sender <queuename> <message> <type>
      type : lz4/plain

Inspect queues with:
  $ sudo rabbitmqctl list_queues

# Dependencies

For C peers:
* rabbitmq-server
* librabbitmq-dev
* liblz4-dev

For Python peers:
* python-lz4
* python-pika

# Notes

A Python peer (sender/receiver) _cannot_ communicate with a C peer
(receiver/sender). That's because LZ4 compressed objects in Python have their
size serialized in the header (4 bytes).

An example:
  >>> import lz4
  >>> a = lz4.compress("hello")
  >>> len(a)
  10
  >>> str(a)
  '\x05\x00\x00\x00Phello'
