#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <unistd.h>


int main(int argc, char **argv)
{

   int sockfd;
   amqp_connection_state_t conn;

   conn = amqp_new_connection();
   sockfd = amqp_open_socket("localhost", 5672);
   amqp_set_sockfd(conn, sockfd);

   amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
   amqp_channel_open(conn, 1);
   amqp_get_rpc_reply(conn);

   amqp_basic_properties_t props = {
       ._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG,
       .content_type = amqp_cstring_bytes("text/plain"),
       .delivery_mode = 2, // persistent mode
   };

   amqp_queue_declare(conn,
                      1,
                      amqp_cstring_bytes("archipelago"),
                      0,
                      1,
                      0,
                      0,
                      AMQP_EMPTY_TABLE);

   int r = amqp_basic_publish(conn,
                              1, //chann id
                              amqp_cstring_bytes(""),
                              amqp_cstring_bytes("archipelago"),
                              0,
                              0,
                              &props,
                              amqp_cstring_bytes("cnanakos1")
                              );
   if (r < 0) {
       fprintf(stderr, "publish error: %s\n", amqp_error_string(-r));
       exit(1);
   }

   amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
   amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
   amqp_destroy_connection(conn);
   return 0;
}
