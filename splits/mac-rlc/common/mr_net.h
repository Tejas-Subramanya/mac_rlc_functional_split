/* Copyright (c) 2016 Kewin Rausch
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

#ifndef __SPLIT_MR_NET_H
#define __SPLIT_MR_NET_H

#define MR_NET_BUF_SIZE		8192

/* Read bytes using the desired socket, getting info of whom is sent such data.
 * Returns the number of bytes sent, or a negative value on error.
 */
int mr_net_recvfrom(
	int                  sockfd,
	char *               buf,
	unsigned int         size,
	struct sockaddr_in * addr,
	socklen_t *          addrlen);

/* Send bytes using the desired socket.
 * Returns the number of bytes sent, or a negative value on error.
 */
int mr_net_sendto(
	int                  sockfd,
	char *               buf,
	unsigned int         size,
	struct sockaddr_in * addr,
	socklen_t            addrlen);

#endif /*__SPLIT_MR_NET_H  */
