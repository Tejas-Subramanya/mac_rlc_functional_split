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
#include <mr_proto.h>
#include <mr_du.h>

#ifdef EBUG
#define DBG(x, ...)			printf("[DU] "x, ##__VA_ARGS__)
#else
#define DBG(x, ...)
#endif

#define MR_DU_CU_ADDR			"192.168.0.150"
#define MR_DU_CU_PORT			9001
#define MR_DU_CU_REC_TIMEOUT		100000000		/* uSec */

/* DU should stop? */
int du_stop = 0;

/* Connected to the CU counterpart? */
int du_con = 0;
/* Socket file descriptor to connect with the CU. */
int du_sockfd = 0;
/* Address to the CU.*/
struct sockaddr_in du_cuaddr = {0};

/* DU thread context. */
pthread_t du_thread;
/* Lock protecting DU threading context. */
pthread_spinlock_t du_lock;

/* Callback invoked when data is received. */
du_recv du_process_data = 0;

/******************************************************************************
 * CU <--> DU logic.                                                          *
 ******************************************************************************/

/* Setup the networking stuff to be used. */
int du_init_net() {
	struct hostent * cu_name;

	du_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if(du_sockfd < 0) {
		printf("Could not open CU socket, error=%d\n", du_sockfd);
		return -1;
	}

	/*
	 * DNS resolving:
	 */

	cu_name = gethostbyname(MR_DU_CU_ADDR);

	if (!cu_name) {
		printf("End-point %s not found!\n", MR_DU_CU_ADDR);
		return -1;
	}

	du_cuaddr.sin_family = AF_INET;
	memcpy(&du_cuaddr.sin_addr.s_addr, cu_name->h_addr, cu_name->h_length);
	du_cuaddr.sin_port = htons(MR_DU_CU_PORT);

	return 0;
}

/* DU central logic. */
void * du_loop(void * args) {
	int  br = 0;
	char buf[MR_NET_BUF_SIZE];

	struct sockaddr_in addr;
	socklen_t          alen = sizeof(struct sockaddr);

	DBG("Starting loop");

	if(du_init_net()) {
		printf("Could not open CU socket, error=%d\n", du_sockfd);
		goto out;
	}

	while(!du_stop) {
		br = mr_net_recvfrom(
			du_sockfd, buf, MR_NET_BUF_SIZE, &addr, &alen);

		if (br > 0 && du_process_data) {
			du_process_data(&addr, buf, br);
		}

		/* Keep going; no pause for you! */
	}

out:	DBG("Loop terminated\n");

	return 0;
}

/******************************************************************************
 * Public procedures.                                                         *
 ******************************************************************************/

int du_init(du_recv process_data) {
	DBG("Starting DU initialization");

	pthread_spin_init(&du_lock, 0);
	du_process_data = process_data;

	/* Create the context where the agent scheduler will run on. */
	if(pthread_create(&du_thread, NULL, du_loop, 0)) {
		printf("DU: Failed to start DU thread.\n");
		return -1;
	}

	return 0;
}

int du_release() {
	du_stop = 1;
	pthread_join(du_thread, 0);

	return 0;
}

int du_send(char * buf, unsigned int len) {
	if(du_sockfd <= 0) {
		return -1;
	}

	return mr_net_sendto(
		du_sockfd, buf, len, &du_cuaddr, sizeof(struct sockaddr_in));
}
