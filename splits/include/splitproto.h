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
 * Protocol wrapper.
 */

#ifndef __SPLIT_PROTOCOLS_H
#define __SPLIT_PROTOCOLS_H

#include <stdint.h>

/* Enumerates the various types of messages exchanged between CU and DU. */
enum split_proto_message_types {
	/* Invalid message! */
	S_PROTO_INVALID = 0,

	/*
	 * MAC-RLC split protocols:
	 */

	/* Request the status of the RLC data. */
	S_PROTO_MR_STATUS_REQ,
	/* Reply to a previous request with the status of the RLC. */
	S_PROTO_MR_STATUS_REP,
	/* Data request type. */
	S_PROTO_MR_DATA_REQ,
	/* Data request reply type. */
	S_PROTO_MR_DATA_REQ_REP,
};

/* Main header of this protocols. */
typedef struct split_head {
	/* Type of the message. */
	uint8_t type;
	/* Version of the message. */
	uint8_t vers;
	/* Length of the data area which follows this header. */
	uint32_t len;
}__attribute__((packed)) sp_head;

/* Pack values of the head into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_pack_head(sp_head * head, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the head is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_identify_head(sp_head ** head, char * buf, uint32_t len);

/******************************************************************************
 * MAC-RLC split protocol.                                                    *
 ******************************************************************************/

/*
 * UL data:
 */

/* Data indicator provides an indication from MAC to RLC that some data has been
 * received and must be take into account by upper layers.
 */
typedef struct split_mr_data_indicator_req {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
	/* First byte of the actual data. */
	uint8_t data;
}__attribute__((packed)) spmr_ireq;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_ireq(spmr_ireq * ireq, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_ireq(spmr_ireq ** ireq, char * buf, uint32_t len);

/*
 * Status:
 */

typedef struct split_mr_status_request {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
	/* Transport block size. */
	uint32_t tb_size;
}__attribute__((packed)) spmr_sreq;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_sreq(spmr_sreq * sreq, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_sreq(spmr_sreq ** sreq, char * buf, uint32_t len);

typedef struct split_mr_status_reply {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
	/* Transport block size. */
	uint32_t tb_size;
	/* Number of bytes. */
	int32_t  bytes;
	/* Number of PDUs. */
	uint32_t pdus;
	/* Frame where it will be sent. */
	uint32_t creation_time;
	/* Remaining bytes left. If the SDU is not segmented, the size of the
	 * SDU itself.
	 */
	uint32_t remaining_bytes;
	/* Indicates if the SDU is segmented. */
	uint32_t segmented;
}__attribute__((packed)) spmr_srep;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_srep(spmr_srep * srep, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_srep(spmr_srep ** srep, char * buf, uint32_t len);


/*
 * DL data:
 */

typedef struct split_mr_data_request {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
}__attribute__((packed)) spmr_dreq;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_dreq(spmr_dreq * dreq, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_dreq(spmr_dreq ** dreq, char * buf, uint32_t len);

typedef struct split_mr_data_reply {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
	/* Data starts from here, and this is it's first elements. */
	uint8_t data;
}__attribute__((packed)) spmr_drep;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_drep(spmr_drep * drep, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_drep(spmr_drep ** drep, char * buf, uint32_t len);

#endif /* __SPLIT_PROTOCOLS_H */
