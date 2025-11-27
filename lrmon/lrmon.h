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
#define RABBITMQ_VHOST		"/"
#define RABBITMQ_USER		"guest"
#define RABBITMQ_PASS		"guest"

/*
 * Queue for managing linked-list of timers.
 */
struct timerq {
    struct timerq	*next;
    int				seconds;
    void			(*func)();
};

/*
 * Local copy of static status.
 */
struct sstatus	{
	char	c1, c2, n1, n2;
	char	fw_h, fw_l;
};

/*
 * Local copy of dynamic status.
 */
struct dstatus	{
	char	radio_state;
	char	tens_of_minutes;
	int		battery_voltage;
	int		npacket_rx;
	int		npacket_tx;
};

extern int				siofd;
extern int				rmqfd;
extern int				maxfd;
extern fd_set			mrfds;
extern struct timeval	tval;

extern int				response_received;
extern int				response_node;
extern int				failure_status;

extern struct sstatus	sstatus;
extern struct dstatus	dstatus;

/*
 * Stubbed out so that we can use libradio.h
 */
typedef unsigned char	uchar_t;
typedef unsigned int	uint_t;

void		main_loop();
void		parse_data(char *);
int			crack(char *, char *[], int, int);

void		response(int, int, int, char *);
void		local_activate();
void		client_activate(int[], int);
void		request_local_status(int);
void		request_client_status(int, int, int);
void		set_time();
void		set_date();
void		set_channel(int, int);
void		send_command(int, int, int, int[], int);
void		reset_controller();

void		state_init();
void		state_machine();
int			state_ready();

void		timer_init();
void		timer_insert(void (*)(), int);
void		timer_remove(void (*)());
void		timer_expiry();
void		timer_dump(char *);

void		sio_init(char *, int);
void		sio_send(char *);
void		process_sio();
void		sio_close();

void		rmq_init(char *);
void		rmq_publish(char *);
