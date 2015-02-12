#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_framing.h>
#include <lz4.h>

#include "example_utils.h"

int main(int argc, char **argv)
{
    int sockfd;
    amqp_connection_state_t conn;
    //amqp_basic_properties_t props;

    const char* const message = "hello hello hello hello hell el o";
    const int message_bytes = strlen(message);

    printf(" <-- Message is: '%s' with size %d\n", message, message_bytes);

    LZ4_stream_t* const lz4_stream = LZ4_createStream();
    char* const cmpBuf = malloc(LZ4_COMPRESSBOUND(100));
    const int cmpBytes =
        LZ4_compress_continue(lz4_stream, message, cmpBuf, strlen(message));

    printf(" <-- Compressed raw data is ");
    print_hex_buffer(cmpBuf, cmpBytes);
    printf("with size %d\n", cmpBytes);

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
    /*
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/lz4");
    props.content_encoding = amqp_cstring_bytes("binary");
    props.delivery_mode = 2; // persistent delivery mode
    */
    amqp_bytes_t data;
    data.bytes = cmpBuf;
    data.len = cmpBytes;

    printf(" <-- AMQP raw data is ");
    print_hex_buffer(data.bytes, data.len);
    printf("with size %lu\n", data.len);

    die_on_error(amqp_basic_publish(conn,
                                    1,
                                    amqp_cstring_bytes(""),
                                    amqp_cstring_bytes("archipelago"),
                                    0,
                                    0,
                                    NULL,
                                    data
                                    ),
                 "Publishing");
    /*
    char* const decBuf = malloc(100);
    const int decBytes = LZ4_decompress_safe(cmpBuf, decBuf, message_bytes, 100);
    */
    printf(" [x] Sent lz4ified '%s'\n", message);

    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                      "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    free(cmpBuf);
    LZ4_freeStream(lz4_stream);

    return 0;
}
