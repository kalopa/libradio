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
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>

#include "lrmon.h"
#include "libradio.h"

int		maxfd;
fd_set	mrfds;

int		crack(char *, char **, int, int);
void	usage();

/*
 *
 */
int
main(int argc, char *argv[])
{
	int i, speed;
	char *device, *rmqhost;

	/*
	 * Process the CLI options.
	 */
	speed = 38400;
	device = "/dev/ttyUSB0";
	rmqhost = strdup("localhost:5672");
	while ((i = getopt(argc, argv, "r:s:l:")) != EOF) {
		switch (i) {
		case 'r':
			rmqhost = optarg;
			break;

		case 's':
			if ((speed = atoi(optarg)) <= 0)
				usage();
			break;

		case 'l':
			device = optarg;
			break;

		default:
			usage();
		}
	}
	openlog("lrmond", LOG_PID, LOG_USER);
	syslog(LOG_INFO, "Local radio control/monitoring service started.");
	maxfd = 0;
	FD_ZERO(&mrfds);
	timer_init();
	sio_init(device, speed);
	rmq_init(rmqhost);
	state_init();
#ifndef DEBUG
	if ((i = fork()) < 0) {
		perror("lrmon: fork");
		exit(1);
	}
	if (i != 0)
		exit(0);
#endif
	main_loop();
	sio_close();
	exit(0);
}

/*
 *
 */
void
main_loop()
{
	int n;
	fd_set rfds;

	while (1) {
		memcpy(&rfds, &mrfds, sizeof(fd_set));
		if ((n = select(maxfd + 1, &rfds, NULL, NULL, &tval)) < 0) {
			perror("lrmon: select");
			exit(1);
		}
		if (tval.tv_sec == 0 && tval.tv_usec == 0)
			timer_expiry();
		if (n == 0)
			continue;
		if (FD_ISSET(siofd, &rfds))
			process_sio();
	}
}

/*
 * Parse the response from the monitoring code.
 */
void
parse_data(char *data)
{
	int chan, node, ticks, cmd;
	char *args[8];

	if (*data != '<') {
		syslog(LOG_DEBUG, "DBG[%s]\n", data);
		return;
	}
	data++;
	if (*data == '+') {
		syslog(LOG_DEBUG, "GOOD RESPONSE!!!!\n");
		failure_status = 0;
		state_machine();
		return;
	}
	if (*data == '-') {
		char *cp;

		if ((cp = strchr(++data, '/')) != NULL)
			*cp++ = '\0';
		if ((failure_status = atoi(data)) < 1)
			failure_status = 99;
		if (cp != NULL)
			node = atoi(cp);
		else
			node = 0;
		syslog(LOG_DEBUG, "FAILURE CODE %d (radio status %d)\n", failure_status, node);
		state_machine();
		return;
	}
	if (crack(data, args, 8, ':') != 4) {
		syslog(LOG_ERR, "Invalid command/response '%s'\n", data);
		return;
	}
	if (args[0] == NULL || (chan = (*args[0] - 'A')) < 0 || chan > 15) {
		syslog(LOG_ERR, "Invalid channel number.\n");
		return;
	}
	if ((node = atoi(args[0] + 1)) < 0 || node > 255) {
		syslog(LOG_ERR, "Invalid node number.\n");
		return;
	}
	if (args[1] == NULL || (ticks = atoi(args[1])) < 0) {
		syslog(LOG_ERR, "Invalid clock ticks.\n");
		return;
	}
	if (args[2] == NULL || (cmd = atoi(args[2])) < 0 || cmd > 255) {
		syslog(LOG_ERR, "Invalid command.\n");
		return;
	}
	switch (cmd) {
	case RADIO_STATUS_RESPONSE:
		failure_status = 0;
		response(chan, node, ticks, args[3]);
		state_machine();
		break;

	default:
		syslog(LOG_ERR, "Unknown command/response.\n");
		syslog(LOG_ERR, "RCVD: Chan %d, node %d, ticks %d, cmd %d, args [%s]\n", chan, node, ticks, cmd, args[3]);
		break;
	}
}

/*
 * Crack a line into its constituent arguments.
 */
int
crack(char *line, char *argv[], int nargs, int delim)
{
	int i;
	char *cp;

	for (i = 0; i < nargs; i++) {
		argv[i] = line;
		if ((cp = strchr(line, delim)) == NULL) {
			i++;
			break;
		}
		*cp++ = '\0';
		line = cp;
	}
	return(i);
}

/*
 *
 */
void
usage()
{
	fprintf(stderr, "Usage: lrmon -s 38400 -l /dev/ttyUSB0\n");
	exit(2);
}
