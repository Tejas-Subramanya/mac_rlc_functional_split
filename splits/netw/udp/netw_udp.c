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

/*
 * Network wrapper implementation with UDP technology.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <netw.h>

#ifdef EBUG
#define NETW_DBG(x, ...)	printf("netw_udp: "x"\n", ##__VA_ARGS__)
#else
#define NETW_DBG(x, ...)
#endif

/* Max size for a single packet. */
#define NETW_UDP_MAX_BUF_SIZE		65507
/* Why? XXX.XXX.XXX.XXX:PPPPP + 1 extra space, just to be sure. */
#define NETW_UDP_MAX_DESTINATION 	22

/* Copy of the destination. */
char netw_dest[NETW_UDP_MAX_DESTINATION] = {0};

/* IPv4 address of the destination. */
char netw_addr[4];
/* Port to be used. */
short netw_port = 0;

/* Socket address to be used while sending. */
struct sockaddr_in netw_senda = {0};

/******************************************************************************
 * Private utilities.                                                         *
 ******************************************************************************/

int nw_parse_udp(char * str, uint32_t len) {
	int ret;
	int tmp[5] = {0};
	char * end = strchr(str, ':');

	/* Copy only the address. */
	memcpy(netw_dest, str, end - str);

	ret = sscanf(str, "%d.%d.%d.%d:%d",
		&tmp[0],
		&tmp[1],
		&tmp[2],
		&tmp[3],
		&tmp[4]);

	netw_addr[0] = (char)tmp[0];
	netw_addr[1] = (char)tmp[1];
	netw_addr[2] = (char)tmp[2];
	netw_addr[3] = (char)tmp[3];

	netw_port    = (short)tmp[4];

	return ret;
}

/******************************************************************************
 * Public API implementation.                                                 *
 ******************************************************************************/

int nw_close(int id) {
	return close(id);
}

int nw_open(char * dest, uint32_t dest_len) {
	int ret = 0;
	int status = 0;

	struct hostent * host;
	struct sockaddr_in addr = {0};

	if(!dest) {
		NETW_DBG("Wrong arguments: dest=%s, len=%d", dest, dest_len);
		return -1;
	}

	if(nw_parse_udp(dest, dest_len) != 5) {
		NETW_DBG("Bad UDP/IP destination format: dest=%s", dest);
		return -1;
	}

	NETW_DBG("Destination set to %s:%d", netw_dest, netw_port);

	host = gethostbyname(netw_dest);

	if(!host) {
		NETW_DBG("Could not resolve host %s", netw_dest);
		return -1;
	}

	netw_senda.sin_family = AF_INET;
	memcpy(&netw_senda.sin_addr.s_addr, host->h_addr, host->h_length);
	netw_senda.sin_port   = htons(netw_port);

	ret = socket(AF_INET, SOCK_DGRAM, 0);

	if(ret < 0) {
		NETW_DBG("Could not open UDP socket, error=%d\n", ret);
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(netw_port);

	status = bind(
		ret,
		(struct sockaddr*)&addr,
		sizeof(struct sockaddr_in));

	/* Bind the address with the socket. */
	if(status) {
		printf("Could not BIND to UDP socket, error=%d\n", status);
		return -1;
	}

	return ret;
}

int nw_recv(int id, char * buf, uint32_t len, uint32_t flags) {
	struct sockaddr_in addr = {0};
	socklen_t alen = 0;

	int f = 0;
	int ret = 0;

	if(flags & NETW_FLAG_NO_WAIT) {
		f = f | MSG_DONTWAIT;
	}

	if(flags & NETW_FLAG_NO_SIGNALS) {
		f = f | MSG_NOSIGNAL;
	}

	ret = recvfrom(id, buf, len, f, (struct sockaddr *)&addr, &alen);

	if(ret < 0) {
		return ret;
	}
#if 0
	/* We are receiving data from someone else ?!?*/
	if(memcmp(&netw_senda.sin_addr.s_addr, &addr.sin_addr.s_addr, 4) != 0) {
		NETW_DBG("Somebody is messing with us from %d.%d.%d.%d",
			addr.sin_addr.s_addr & 0xff000000,
			addr.sin_addr.s_addr & 0x00ff0000,
			addr.sin_addr.s_addr & 0x0000ff00,
			addr.sin_addr.s_addr & 0x000000ff);
		return -1;
	}
#endif
	return ret;
}

int nw_send(int id, char * buf, uint32_t len, uint32_t flags) {
	socklen_t alen = sizeof(struct sockaddr_in);

	int f = 0;
	int ret = 0;

	if(len > NETW_UDP_MAX_BUF_SIZE) {
		NETW_DBG("Data is too large, length=%d", len);
		return -1;
	}

	if(flags & NETW_FLAG_NO_WAIT) {
		f = f | MSG_DONTWAIT;
	}

	if(flags & NETW_FLAG_NO_SIGNALS) {
		f = f | MSG_NOSIGNAL;
	}

	ret = sendto(id, buf, len, f, (struct sockaddr *)&netw_senda, alen);

	if(ret < 0) {
		return ret;
	}

	return ret;
}
