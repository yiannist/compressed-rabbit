#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_framing.h>
#include <lz4.h>

#include "utils.h"

int main(int argc, char const *const *argv)
{
    int sockfd;
    amqp_connection_state_t conn;
    amqp_bytes_t queuename = amqp_cstring_bytes(argv[1]);

    // AMQP stuff
    conn = amqp_new_connection();

    die_on_error(sockfd = amqp_open_socket("localhost", 5672),
                 "Opening socket");
    amqp_set_sockfd(conn, sockfd);

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                 "guest", "guest"),
                      "Loggin in");
    amqp_channel_open(conn, 1);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

    amqp_queue_declare(conn,
                       1,
                       queuename,
                       0,
                       1, // durable
                       0,
                       0,
                       amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");

    amqp_basic_consume(conn,
                       1,
                       queuename,
                       amqp_empty_bytes,
                       0,
                       1,
                       0,
                       amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    printf(" [*] Waiting for messages. To exit press CTRL+C\n");

    while (1) {
        char *cmpBuf;
        int cmpBytes, message_bytes;
        amqp_rpc_reply_t res;
        amqp_envelope_t envelope;
        LZ4_streamDecode_t* const lz4_stream_decode = LZ4_createStreamDecode();
        char *message = malloc(LZ4_COMPRESSBOUND(100));

        amqp_maybe_release_buffers(conn);
        res = amqp_consume_message(conn, &envelope, NULL, 0);
        if (AMQP_RESPONSE_NORMAL != res.reply_type) {
            break;
        }

        cmpBuf = (char *) envelope.message.body.bytes;
        cmpBytes = (int) envelope.message.body.len;
        printf(" --> Received raw data: ");
        print_hex_buffer(cmpBuf, cmpBytes);
        printf("with size %d\n", cmpBytes);

        if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
            if (!strcmp("lz4",
                        envelope.message.properties.content_type.bytes)) {
                printf(" --> Data is LZ4-compressed. Trying to uncompress"
                       "...\n");
                message_bytes = LZ4_decompress_safe_continue(lz4_stream_decode,
                                                             cmpBuf, message,
                                                             cmpBytes, 100);
            } else {
                message_bytes = cmpBytes;
                message = cmpBuf;
            }
        }
        printf(" [x] Received '%s' with size %d\n", message, message_bytes);

        free(message);
        amqp_destroy_envelope(&envelope);
        LZ4_freeStreamDecode(lz4_stream_decode);
    }

    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                      "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    return 0;
}
