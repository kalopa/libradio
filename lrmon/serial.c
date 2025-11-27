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
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <syslog.h>

#include "lrmon.h"

/*
 * Translate baud rates into internal value.
 */
struct	speed	{
	int	value;
	int	code;
} speeds[] = {
	{50, B50},
	{75, B75},
	{110, B110},
	{134, B134},
	{150, B150},
	{200, B200},
	{300, B300},
	{600, B600},
	{1200, B1200},
	{1800, B1800},
	{2400, B2400},
	{4800, B4800},
	{9600, B9600},
	{19200, B19200},
	{38400, B38400},
	{57600, B57600},
	{115200, B115200},
	{230400, B230400},
	{0, 0}
};

int		siofd;
int		offset;
char	buffer[1024];

/*
 *
 */
void
sio_init(char *device, int speed)
{
	int i;
	struct termios tios;

	/*
	 * Initialize communications.
	 */
	for (i = 0; speeds[i].value != 0; i++)
		if (speeds[i].value == speed)
			break;
	if (speeds[i].value == 0) {
		fprintf(stderr, "lrmon: sio_init: invalid baud rate: %d\n", speed);
		exit(1);
	}
	speed = speeds[i].code;
	if ((siofd = open(device, O_RDWR|O_NOCTTY|O_NDELAY)) < 0) {
		fprintf(stderr, "lrmon: sio_init open: ");
		perror(device);
		exit(1);
	}
	if (tcgetattr(siofd, &tios) < 0) {
		perror("lrmon: tcgetattr");
		exit(1);
	}
	tios.c_cflag &= ~(CSIZE|PARENB);
	tios.c_cflag |= (CLOCAL|CREAD|CS8);
	tios.c_lflag = tios.c_iflag = tios.c_oflag = 0;
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 1;
	if (speed != 0) {
		cfsetispeed(&tios, speed);
		cfsetospeed(&tios, speed);
	} else {
		tios.c_lflag |= ISIG;
	}
	if (tcsetattr(siofd, TCSANOW, &tios) < 0) {
		perror("lrmon: tcsetattr");
		exit(1);
	}
	FD_SET(siofd, &mrfds);
	if (maxfd < siofd)
		maxfd = siofd;
	/*
	 * If we're in bootstrap mode, reset the system (otherwise this is ignored).
	 */
	if (write(siofd, "R\r\n", 3) != 3) {
		perror("lrmon: initial write failure");
		exit(1);
	}
	offset = 0;
}

/*
 * Send a string to the serial device.
 */
void
sio_send(char *msg)
{
	int len;

	len = strlen(msg);
	syslog(LOG_DEBUG, "Send [%s]\n", msg);
	if (write(siofd, msg, len) != len) {
		perror("lrmon: sio_send write msg");
		exit(1);
	}
	if (write(siofd, "\r\n", 2) != 2) {
		perror("lrmon: sio_send write crlf");
		exit(1);
	}
}

/*
 *
 */
void
process_sio()
{
	int n;
	char *cp, *startp;

	if ((n = read(siofd, buffer + offset, sizeof(buffer) - offset - 1)) < 0) {
		perror("lrmon: sio read");
		exit(1);
	}
	if (n == 0)
		return;
	offset += n;
	buffer[offset] = '\0';
	startp = buffer;
	while (1) {
		if ((cp = strpbrk(startp, "\r\n")) == NULL)
			break;
		*cp++ = '\0';
		if (*startp != '\0')
			parse_data(startp);
		/*
		 * Treat a CR/LF pair as just a newline.
		 */
		while (*cp == '\r' || *cp == '\n')
			cp++;
		startp = cp;
	}
	/*
	 * We're looking to see if there's leftover data. If not, then
	 * reset the offset.  Likewise, if we haven't see a CR/LF in
	 * half a buffer-load, dump the buffer.
	 */
	for (cp = buffer; *startp != '\0'; cp++, startp++)
		*cp = *startp;
	offset = cp - buffer;
}

/*
 *
 */
void
sio_close()
{
	FD_CLR(siofd, &mrfds);
	close(siofd);
}
