/* Copyright (c) 2017 Kewin Rausch and Tejas Subramanya
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <pthread.h>

#include "netw.h"
#include "splitproto.h"

#include "mr_du.h"

#define EBUG

#ifdef EBUG
#define DU_DBG(x, ...)			printf(			\
"[DU] "x"\n", ##__VA_ARGS__)
#else
#define DU_DBG(x, ...)
#endif

#define DU_BUF_SIZE		8192

/* DU should stop? */
int du_stop = 0;

/* Socket file descriptor for sending/receiving data. */
int du_sockfd = 0;

/* DU thread context. */
pthread_t du_thread;
/* Lock protecting DU threading context. */
pthread_spinlock_t du_lock;

/* Callback invoked when data is received. */
int (* du_process_data) (char * buf, unsigned int len) = 0;

/******************************************************************************
 * DU <--> CU logic.                                                          *
 ******************************************************************************/

/* DU central logic. */
void * du_loop(void * args) {
	int  br = 0;
	char buf[DU_BUF_SIZE] = {0};

	DU_DBG("Starting DU loop");

	while(!du_stop) {
		br = nw_recv(
			du_sockfd,
			buf, DU_BUF_SIZE,
			NETW_FLAG_NO_SIGNALS /*| NETW_FLAG_NO_WAIT*/);

		if (br > 0 && du_process_data) {
			du_process_data(buf, br);
		}

		/* Pause only if there's nothing to do. */
		//if(br < 0) {
		//	usleep(50);
		//}
	}

	DU_DBG("DU Loop terminated\n");

	return 0;
}

/******************************************************************************
 * Public procedures.                                                         *
 ******************************************************************************/

int du_init(char * args, int (* process_data) (char * buf, unsigned int len)) {
	DU_DBG("Starting DU initialization");

	du_sockfd = nw_open(args, strlen(args));

	if(du_sockfd < 0) {
		DU_DBG("Could not open DU socket.");
		return -1;
	}

	pthread_spin_init(&du_lock, 0);
	du_process_data = process_data;

	/* Create the context where the agent scheduler will run on. */
	if(pthread_create(&du_thread, NULL, du_loop, 0)) {
		DU_DBG("Failed to start DU thread.");
		return -1;
	}

	return 0;
}

int du_release() {
	du_stop = 1;
	pthread_join(du_thread, 0);

	nw_close(du_sockfd);

	return 0;
}

int du_send(char * buf, unsigned int len) {
	if(du_sockfd <= 0) {
		return -1;
	}

	/* Waiting send operation. */
	return nw_send(du_sockfd, buf, len, NETW_FLAG_NO_SIGNALS);
}
