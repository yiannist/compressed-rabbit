#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <amqp.h>
#include <amqp_framing.h>
#include <lz4.h>

#include "utils.h"

int main(int argc, char **argv)
{
    int sockfd, compress;
    amqp_connection_state_t conn;
    amqp_bytes_t queuename;
    char message[COMP_BUF_SIZE];
    char *type, *cmpBuf;
    int message_bytes, cmpBytes;
    LZ4_stream_t* const lz4_stream = LZ4_createStream();
    struct timespec tik, tok, tic, toc;
    double diff;
    char *file_pathname = NULL;
    FILE *fp;

    if (argc < 4) {
        printf("Usage: ./sender <queuename> <type> <message-file>\n"
               "    type : lz4/plain\n");
        exit(1);
    }

    queuename = amqp_cstring_bytes(argv[1]);
    type = argv[2];
    file_pathname = argv[3];

    fp = fopen(file_pathname, "r");
    fscanf(fp, "%s", message);
    message_bytes = strlen(message) + 1;

/*
    printf(" <-- Message is: '%s' with size %d (= %d + \\0)\n", message,
           message_bytes, message_bytes - 1);
    printf(" <-- Raw data is ");
    print_hex_buffer((char *) message, message_bytes);
    printf(" with size %d\n", message_bytes);
*/

    compress = !strcmp("lz4", type);
    if (compress) {
        //printf(" <-- Data should be compressed.\n");
        cmpBuf = malloc(LZ4_COMPRESSBOUND(COMP_BUF_SIZE));

        clock_gettime(CLOCK_REALTIME, &tic);
        cmpBytes =
            LZ4_compress_continue(lz4_stream, message, cmpBuf, strlen(message));

        clock_gettime(CLOCK_REALTIME, &toc);
        diff = (toc.tv_sec - tic.tv_sec) + (toc.tv_nsec - tic.tv_nsec)/1E9;
    } else {
        cmpBuf = (char *) message;
        cmpBytes = message_bytes;
    }

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

    // Put the channel in publisher-confirm mode
    amqp_confirm_select(conn, 1);

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes(type);
    props.delivery_mode = 2; // persistent delivery mode

    amqp_bytes_t data;
    data.bytes = cmpBuf;
    data.len = cmpBytes;

/*
    printf(" <-- AMQP raw data is ");
    print_hex_buffer(data.bytes, data.len);
    printf(" with size %lu\n", data.len);
*/

    amqp_frame_t decoded_frame;
    clock_gettime(CLOCK_REALTIME, &tik);
    amqp_basic_publish(conn,
                       1,
                       amqp_cstring_bytes(""),
                       queuename,
                       0,
                       0,
                       &props,
                       data
                       );
    amqp_simple_wait_frame(conn, &decoded_frame);
    clock_gettime(CLOCK_REALTIME, &tok);

    //printf(" [x] Sent '%s' to '%s' queue\n", message, (char *) queuename.bytes);

    // Handling ACK as suggested:
    // https://groups.google.com/forum/#!topic/rabbitmq-users/LOEBwpsE0kA
    if (decoded_frame.frame_type == AMQP_FRAME_METHOD && decoded_frame.channel == 1) {
        if (decoded_frame.payload.method.id == AMQP_BASIC_RETURN_METHOD) {
            /* Message was published with mandatory = true and the message
             * wasn't routed to a queue, so the message is returned */
            amqp_message_t returned_message;
            die_on_amqp_error(amqp_read_message(conn, 1, &returned_message, 0),
                              "Message wasn't routed to a queue");
            /* Do something with returned, free memory when done */
            amqp_destroy_message(&returned_message);

            /* look for the AMQP_BASIC_ACK_METHOD from the broker */
            die_on_error(amqp_simple_wait_frame(conn, &decoded_frame),
                         "Looking for AMQP_BASIC_ACK_METHOD from the broker FAILED");
            if (decoded_frame.frame_type != AMQP_FRAME_METHOD || decoded_frame.channel != 1) {
                /* something is probably wrong... handle it */
                printf("Something bad happened\n");
            }
        }
        if (decoded_frame.payload.method.id == AMQP_BASIC_ACK_METHOD) {
            //amqp_basic_ack_t *a = (amqp_basic_ack_t*)decoded_frame.payload.method.decoded;
            /* if you've kept a count of the messages you've published on the channel,
             * the a->delivery_tag is the message serial being acknowledged.
             * if a->multiple != 0, that means all messages up-to-and-including that message
             * serial are being acknowledged */
            printf(" [i] Message ACK'd\n");
        } else {
            /* You've received a different method, probably not what you want */
            printf(" [i] Strange method received\n");
        }
    }

    printf(" [i] Compression latency was: %lf\n", diff);

    diff = (tok.tv_sec - tik.tv_sec) + (tok.tv_nsec - tik.tv_nsec)/1E9;

    printf(" [i] Communication latency was: %lf\n", diff);

    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
                      "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
                      "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    if (compress) {
        free(cmpBuf);
    }
    LZ4_freeStream(lz4_stream);

    return 0;
}
