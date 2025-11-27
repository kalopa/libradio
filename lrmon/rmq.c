/*
 * Copyright (c) 2024, Kalopa Robotics Limited.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
-- Installing: /usr/local/include/rabbitmq-c/amqp.h
-- Installing: /usr/local/include/rabbitmq-c/framing.h
-- Installing: /usr/local/include/rabbitmq-c/tcp_socket.h
-- Installing: /usr/local/include/rabbitmq-c/ssl_socket.h
-- Installing: /usr/local/include/rabbitmq-c/export.h
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

#include "lrmon.h"

int							rmqfd;
amqp_connection_state_t		conn;
amqp_socket_t				*socket;

/*
 *
 */
void
rmq_init(char *rmq_host)
{
	int rmqfd, port, status;
	char *cp;
	amqp_rpc_reply_t reply;

	printf("Begin... [%s]\n", rmq_host);
	if ((cp  = strchr(rmq_host, ':')) != NULL) {
		printf("Got colon... [%s]\n", cp);
		*cp++ = '\0';
		port = atoi(cp);
	} else
		port = 5672;
	syslog(LOG_DEBUG, "RMQ Host: [%s], port %d\n", rmq_host, port);
	conn = amqp_new_connection();
	if ((socket = amqp_tcp_socket_new(conn)) == NULL) {
		syslog(LOG_ERR, "amqp_tcp_socket_new failed\n");
		exit(1);
	}
	if ((status = amqp_socket_open(socket, rmq_host, port)) != AMQP_STATUS_OK) {
		syslog(LOG_ERR, "amqp_socket_open failed: %d\n", status);
		exit(1);
	}
	printf("Got status: %d\n", status);
	reply = amqp_login(conn, RABBITMQ_VHOST, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
						RABBITMQ_USER, RABBITMQ_PASS);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		syslog(LOG_ERR, "amqp_login failed\n");
		exit(1);
	}
	printf("logged in\n");
	if ((rmqfd = amqp_get_sockfd(conn)) < 0) {
		syslog(LOG_ERR, "amqp_get_sockfd failed\n");
		exit(1);
	}
	printf("Got socket fd: %d\n", rmqfd);
	FD_SET(rmqfd, &mrfds);
	if (maxfd < rmqfd)
		maxfd = rmqfd;
}

/*
 * Publish a message to the outbound RMQ channel.
 */
void
rmq_publish(char *msg)
{
	syslog(LOG_DEBUG, "PUB: %s\n", msg);
#if 0
	amqp_basic_properties_t props;
	amqp_bytes_t body;
	amqp_rpc_reply_t reply;
	amqp_channel_t channel = 1;
	amqp_boolean_t mandatory = 0;
	amqp_boolean_t immediate = 0;

	body.bytes = msg;
	body.len = strlen(msg);
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2;
	reply = amqp_basic_publish(conn, channel, amqp_cstring_bytes("amq.topic"),
							   amqp_cstring_bytes("lrmon"), mandatory, immediate, &props, body);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		syslog(LOG_ERR, "amqp_basic_publish failed\n");
		exit(1);
	}
#endif
}
