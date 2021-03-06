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

	/* Request the status of all RLC logical channels buffer(Control and Data plane) along with the actual data. */
	S_PROTO_MR_STATUS_DATA_REQ,
	/* Reply to a previous request with the status and data from all logical channel buffers. */
	S_PROTO_MR_STATUS_DATA_REP,
	/* Data Ind type. */
	S_PROTO_MR_DATA_IND,
        /* RRC Data Req type. */
        S_PROTO_MR_RRC_DATA_REQ,
        /* RRC Data request reply type. */
        S_PROTO_MR_RRC_DATA_REQ_REP,
        /* RRC Data Ind type. */
        S_PROTO_MR_RRC_DATA_IND,
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
 * UL data MAC->RLC
 */

/* MAC-RLC Data indicator provides an indication from MAC to RLC that some data has been
 * received and must be take into account by upper layers.
 */
typedef struct split_mr_data_indicator_req {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. */
	uint32_t channel;
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

/* 
 * UL data MAC->RRC
 */

/* MAC-RRC Data indicator provides an indication from MAC to RRC that some data
 * has been received and must be taken into account by RRC.
 */

typedef struct split_mr_data_indicator_rrc_req {
	/* Carrier id. */
	int32_t CC_id;
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the req is issued. */
	uint32_t frame;
	/* Subframe where the req is issued. */
	uint32_t subframe;
	/* Signalling Radio Bearer id. */
	uint16_t srb_id;
}__attribute__((packed)) spmr_rrc_ireq;


/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_rrc_ireq(spmr_rrc_ireq * rrc_ireq, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_rrc_ireq(
	spmr_rrc_ireq ** rrc_ireq, char * buf, uint32_t len);


typedef struct split_mr_status_data_request {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channel where the request is issued. When the request is issued for one channel, 
	 * respond with status and data from all channels */
	uint32_t channel;
	/* Transport block size. */
	uint32_t tb_size;
}__attribute__((packed)) spmr_sreq_dreq;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_sreq_dreq(spmr_sreq_dreq * sreq_dreq, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_sreq_dreq(spmr_sreq_dreq ** sreq_dreq, char * buf, uint32_t len);

typedef struct split_mr_status_data_reply {
	/* RNTI id. */
	uint16_t rnti;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Channels where the request is issued.3 SRB and 11 DRB */
	uint32_t channel[14];
	/* Transport block size. */
	uint32_t tb_size[14];
	/* Number of bytes. */
	int32_t  bytes[14];
	/* Number of PDUs. */
	uint32_t pdus[14];
	/* Frame where it will be sent. */
	uint32_t creation_time[14];
	/* Remaining bytes left. If the SDU is not segmented, the size of the
	 * SDU itself.
	 */
	uint32_t remaining_bytes[14];
	/* Indicates if the SDU is segmented. */
	uint32_t segmented[14];
}__attribute__((packed)) spmr_srep_drep;

/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_srep_drep(spmr_srep_drep * srep_drep, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_srep_drep(spmr_srep_drep ** srep_drep, char * buf, uint32_t len);

/*
 * DL data RRC->MAC
*/

typedef struct split_mr_rrc_data_request {
	/* Carrier id. */
	int32_t CC_id;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Signalling Radio Bearer id. */
	uint16_t srb_id;
	/* Number of transport blocks. */
	uint8_t num_tb;
}__attribute__((packed)) spmr_rrc_dreq;


/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_rrc_dreq(spmr_rrc_dreq * rrc_dreq, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_rrc_dreq(
	spmr_rrc_dreq ** rrc_dreq, char * buf, uint32_t len);

typedef struct split_mr_rrc_data_reply {
	/* Carrier id. */
	int32_t CC_id;
	/* Frame where the request is issued. */
	uint32_t frame;
	/* Signalling Radio Bearer id. */
	uint16_t srb_id;
}__attribute__((packed)) spmr_rrc_drep;


/* Pack values of the structure into the given buffer.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_pack_rrc_drep(spmr_rrc_drep * rrc_drep, char * buf, uint32_t len);

/* Adjust the given pointer to point to the area where the structure is located.
 *
 * Return 0 on success, otherwise a negative error code.
 */
int sp_mr_identify_rrc_drep(
	spmr_rrc_drep ** rrc_drep, char * buf, uint32_t len);

/******************************************************************************
 * End of MAC-RLC split protocol.                                             *
 ******************************************************************************/
        
#endif /* __SPLIT_PROTOCOLS_H */
