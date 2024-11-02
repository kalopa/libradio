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
#include <sys/types.h>
#include <syslog.h>

#include "lrmon.h"

#define DEFAULT_TIMEOUT		60

struct timerq	*timerq;
struct timerq	*tq_free;
struct timeval	tval;

/*
 *
 */
void
timer_init()
{
	timerq = tq_free = NULL;
	tval.tv_sec = DEFAULT_TIMEOUT;
	tval.tv_usec = 0;
}

/*
 *
 */
void
timer_insert(void (*func)(), int secs)
{
	struct timerq *tqp;

	//syslog(LOG_DEBUG, "Allocating timer for %d seconds\n", secs);
	//timer_dump("Alloc queue");
	if (tq_free != NULL) {
		//syslog(LOG_DEBUG, "Off freelist\n");
		tqp = tq_free;
		tq_free = tqp->next;
	} else {
		if ((tqp = (struct timerq *)malloc(sizeof(struct timerq))) == NULL) {
			perror("timer_insert: malloc");
			exit(1);
		}
	}
	tqp->seconds = secs;
	tqp->func = func;
	tqp->next = NULL;
	/*
	 * Now do an insertion-sort on the timer queue. The queue is sorted in
	 * ascending order of the number of seconds remaining, relative to the
	 * last step.
	 */
	if (timerq == NULL || timerq->seconds > tqp->seconds) {
		/*
		 * Insert at the head of the queue.
		 */
		if (timerq != NULL)
			timerq->seconds -= tqp->seconds;
		tqp->next = timerq;
		timerq = tqp;
		tval.tv_sec = tqp->seconds;
		tqp->seconds = 0;
		tval.tv_usec = 0;
	} else {
		struct timerq *ntqp;

		/*
		 * Find the insert point, decrementing the relative seconds at each step.
		 */
		for (ntqp = timerq; ntqp->next != NULL; ntqp = ntqp->next) {
			if (ntqp->next->seconds > tqp->seconds)
				break;
			tqp->seconds -= ntqp->seconds;
		}
		tqp->next = ntqp->next;
		ntqp->next = tqp;
	}
	//timer_dump("New queue");
}

/*
 * Remove any timers which call the given function.
 */
void
timer_remove(void (*func)())
{
	struct timerq *tqp, *tqpp;

	//syslog(LOG_DEBUG, "Removing timer for %x\n", func);
	//timer_dump("Remove queue");
	for (tqpp = NULL, tqp = timerq; tqp != NULL; tqpp = tqp, tqp = tqp->next) {
		if (tqp->func == func) {
			if (tqpp == NULL)
				timerq = tqp->next;
			else
				tqpp->next = tqp->next;
			tqp->next = tq_free;
			tq_free = tqp;
			break;
		}
	}
	//timer_dump("New queue");
}

/*
 * The timer has expired. Run all of the functions at the head of the queue
 * which have expired.
 */
void
timer_expiry()
{
	struct timerq *runq, *tqp;

	/*
	 * Split the timer queue into a runnable queue and a timer queue lest
	 * any running function want to manipulate the timer queue.
	 */
	//syslog(LOG_DEBUG, "Running the queue...\n");
	//timer_dump("Initial queue");
	if ((runq = timerq) == NULL) {
		//syslog(LOG_DEBUG, "No functions to run\n");
		tval.tv_sec = DEFAULT_TIMEOUT;
		tval.tv_usec = 0;
		return;
	}
	while (runq->next != NULL && runq->next->seconds == 0)
		runq = runq->next;
	if ((timerq = runq->next) != NULL) {
		tval.tv_sec = timerq->seconds;
		timerq->seconds = 0;
	} else
		tval.tv_sec = DEFAULT_TIMEOUT;
	runq->next = NULL;
	/*
	 * Now, execute the run queue.
	 */
	while (runq != NULL) {
		tqp = runq;
		runq = runq->next;
		tqp->next = NULL;
		syslog(LOG_DEBUG, "Calling func...\n");
		tqp->func();
		tqp->next = tq_free;
		tq_free = tqp;
	}
	syslog(LOG_DEBUG, "Execution finished! timeout is now %ld\n", tval.tv_sec);
}

/*
 * Dump the timer queue.
 */
void
timer_dump(char *msg)
{
	struct timerq *tqp;

	if (msg != NULL)
		syslog(LOG_DEBUG, "%s:\n", msg);
	for (tqp = timerq; tqp != NULL; tqp = tqp->next)
		syslog(LOG_DEBUG, "  %x(%ds)\n", tqp->func, tqp->seconds);
	syslog(LOG_DEBUG, "tval: %ld/%ld\n", tval.tv_sec, tval.tv_usec);
}
