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
#include <netdb.h>
#include <netinet/in.h>

#include <pthread.h>

#include <mr_net.h>
#include <mr_cu.h>

#ifdef EBUG
#define DBG(x, ...)			printf("[CU]     "x"\n", ##__VA_ARGS__)
#else
#define DBG(x, ...)
#endif

#define MR_CU_PORT			9001
#define MR_CU_CLIENTS			1

/* CU should stop? */
int cu_stop = 0;

/* Socket file descriptor which is listening for DUs. */
int cu_sockfd = 0;
/* Address of the last CU sender. */
struct sockaddr_in cu_duaddr = {0};

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
	int status = 0;

	int br = 0;
	char buf[MR_NET_BUF_SIZE];

	struct sockaddr_in addr;
	socklen_t clen = sizeof(struct sockaddr);

	DBG("Starting loop");

	cu_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if(cu_sockfd < 0) {
		printf("Could not open CU socket, error=%d\n", cu_sockfd);
		goto out;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(MR_CU_PORT);

	status = bind(
		cu_sockfd,
		(struct sockaddr*)&addr,
		sizeof(struct sockaddr_in));

	/* Bind the address with the socket. */
	if(status) {
		printf("Could not BIND to CU socket, error=%d\n", status);
		perror("bind");

		goto out;
	}

	DBG("Listening for incoming data...");

	while(!cu_stop) {
		br = mr_net_recvfrom(
			cu_sockfd, buf, MR_NET_BUF_SIZE, &cu_duaddr, &clen);

		/* Something has been read. */
		if(br > 0 && cu_process_data) {
			cu_process_data(&cu_duaddr, buf, br);
		}
	}

out:	DBG("Loop terminated\n");

	return 0;
}

/******************************************************************************
 * Public procedures.                                                         *
 ******************************************************************************/

int cu_init(cu_recv process_data) {
	DBG("Starting CU initialization");

	pthread_spin_init(&cu_lock, 0);
	cu_process_data = process_data;

	/* Create the context where the agent scheduler will run on. */
	if(pthread_create(&cu_thread, NULL, cu_loop, 0)) {
		printf("CU: Failed to start DU thread.\n");
		return -1;
	}

	return 0;
}

int cu_release() {
	DBG("Starting CU releasing");

	cu_stop = 1;
	pthread_join(cu_thread, 0);

	return 0;
}

int cu_send(char * buf, unsigned int len) {
	if(cu_sockfd <= 0) {
		return -1;
	}

	return mr_net_sendto(
		cu_sockfd, buf, len, &cu_duaddr, sizeof(struct sockaddr_in));
}

