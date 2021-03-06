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

#ifndef __MR_DU_H
#define __MR_DU_H

#include <netw.h>
#include <splitproto.h>

#define DU_BUF_SIZE   8192

 extern pthread_spinlock_t du_lock;

/* Initialize the DU mechanisms. Data received through the CU mechanism will be
 * passed to the given callback, in order to allow custom processing.
 *
 * Returns 0 on success, otherwise a negative error code.
 */
int du_init(char * args, int (* process_data) (char * buf, unsigned int len));

/* Release the DU mechanisms.
 *
 * Returns 0 on success, otherwise a negative error code.
 */
int du_release(void);

/* Send some bytes to the CU. Such bytes will be sent as they are, so is up to
 * you provide the necessary HW header.
 *
 * Returns the number of bytes sent on success, otherwise a negative error code.
 */
int du_send(char * buf, unsigned int len);

#endif
