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
 * This header resumes the format of the messages exchanged by the Mac-Rlc
 * split logic.
 */

#ifndef __SPLIT_MR_PROTO_H
#define __SPLIT_MR_PROTO_H

#include <stdint.h>

/* Enumerates the various types of messages exchanged between CU and DU. */
enum mr_proto_message_types {
	/* Invalid message! */
	MR_PROTO_INVALID = 0,
	/* Data request type. */
	MR_PROTO_DATA_REQ,
	/* Data response type. */
	MR_PROTO_DATA_RES,
};

/* Main header of elements exchanged between CU and DU nodes. */
struct mr_proto {
	/* Type of the message. */
	uint8_t type;
	/* Version of the message. */
	uint8_t vers;
	/* Length of the data area which follows this header. */
	uint32_t len;
}__attribute__((packed));


/******************************************************************************
 * Data request.                                                              *
 ******************************************************************************/


/* This structure defines how a data request is organized. */
struct mr_proto_data_req {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
}__attribute__((packed));

/* This structure defines how a reply to a data request is organized. */
struct mr_proto_data_rep {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
	/* Data starts from here, and this is it's first elements. */
	uint8_t data;
}__attribute__((packed));

/* Parses a buffer to extract data request information.
 * Returns 0 on success, otherwise a negative error number.
 */
int proto_parse_data_req(
	struct mr_proto ** head,
	struct mr_proto_data_req ** req,
	char *   msg,
	int      len);

/* Prepares a data request message in a given buffer.
 * Returns 0 on success, otherwise a negative error number.
 */
int proto_prepare_data_req(
	uint16_t rnti,
	uint32_t frame,
	uint32_t channel,
	char *   buf,
	int      len);

#endif /*__SPLIT_MR_PROTO_H  */
