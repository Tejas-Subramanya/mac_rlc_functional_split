/* Copyright (c) 2017 Kewin Rausch
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

#include <sys/ioctl.h>
#include <pthread.h>

#include <mr_cu.h>

#ifdef EBUG
#define CU_DBG(x, ...)			printf("cu: "x"\n", ##__VA_ARGS__)
#else
#define CU_DBG(x, ...)
#endif

#define CU_BUF_SIZE			8192


/* CU should stop? */
int cu_stop = 0;

/* Socket file descriptor for receiving/sending. */
int cu_sockfd = 0;

/* DU thread context. */
pthread_t cu_thread;
/* Lock protecting CU threading context. */
pthread_spinlock_t cu_lock;

/* Callback invoked when data is received. */
cu_recv cu_process_data = 0;

/******************************************************************************
 * CU <--> DU logic.                                                          *
 ******************************************************************************/


/* CU central logic. */
void * cu_loop(void * args) {
	int br = 0;
	char buf[CU_BUF_SIZE] = {0};

	CU_DBG("Starting loop");

	while(!cu_stop) {
		br = nw_recv(
			cu_sockfd,
			buf, CU_BUF_SIZE,
			NETW_FLAG_NO_SIGNALS | NETW_FLAG_NO_WAIT);

		/* Something has been read. */
		if(br > 0 && cu_process_data) {
			cu_process_data(buf, br);
		}

		/* Keep going; no pause for you! */
	}

	CU_DBG("Loop terminated\n");

	return 0;
}

/******************************************************************************
 * Public procedures.                                                         *
 ******************************************************************************/

int cu_init(char * dest, cu_recv process_data) {
	CU_DBG("Starting CU initialization");

	cu_sockfd = nw_open(dest, strlen(dest));

	if(cu_sockfd < 0) {
		CU_DBG("Could not open CU socket.");
		return -1;
	}

	pthread_spin_init(&cu_lock, 0);
	cu_process_data = process_data;

	/* Create the context where the agent scheduler will run on. */
	if(pthread_create(&cu_thread, NULL, cu_loop, 0)) {
		CU_DBG("CU: Failed to start DU thread.\n");
		return -1;
	}

	return 0;
}

int cu_release() {
	CU_DBG("Starting CU releasing");

	cu_stop = 1;
	pthread_join(cu_thread, 0);

	nw_close(cu_sockfd);

	return 0;
}

int cu_send(char * buf, unsigned int len) {
	if(cu_sockfd <= 0) {
		return -1;
	}

	/* Waiting send operation. */
	return nw_send(cu_sockfd, buf, len, NETW_FLAG_NO_SIGNALS);
}
