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

/*
 * Network wrapper; abstract over the technology used for split communication.
 */

#ifndef __SPLITS_NETWRAP_H
#define __SPLITS_NETWRAP_H

#include <stdint.h>

/* Flags that personalize recv/send operations. */
enum netw_IO_flags {
	NETW_FLAG_NO_WAIT    = 1,
	NETW_FLAG_NO_SIGNALS = 2
};

/* Close an already opened communication channel.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int nw_close(int id);

/* Open a communication channel with the desired destination.
 *
 * Returns an id to use with nw operations, otherwise a negative error code.
 */
int nw_open(char * dest, uint32_t dest_len);

/* Receive data from the open nw identifier.
 *
 * Returns the number of bytes read, or a negative error code.
 */
int nw_recv(int id, char * buf, uint32_t len, uint32_t flags);

/* Send data to the open nw identifier.
 *
 * Returns the number of bytes sent, or a negative error code.
 */
int nw_send(int id, char * buf, uint32_t len, uint32_t flags);

#endif /* __SPLITS_NETWRAP_H */
