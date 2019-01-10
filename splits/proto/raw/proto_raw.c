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
 * RAW Protocol wrapper; will use straight packed structures to do the job.
 */

#include <string.h>

#include <splitproto.h>

/* Generically pack bytes. */
static inline int sp_generic_pack(
	void *   data,
	uint32_t off,
	uint32_t data_len,
	char *   buf,
	uint32_t len) {

	if(off + data_len > len) {
		return -1;
	}

	memcpy(buf + off, data, data_len);

	return off + data_len;
}

/* Generically pack bytes. */
static inline int sp_generic_identify(
	void **  data,
	uint32_t off,
	char *   buf,
	uint32_t len) {

	if(off > len) {
		return -1;
	}

	*data = (void *)(buf + off);

	return 0;
}

/******************************************************************************
 * Public API implementation.                                                 *
 ******************************************************************************/

int sp_pack_head(sp_head * head, char * buf, uint32_t len) {
	return sp_generic_pack(head, 0, sizeof(sp_head), buf, len);
}

int sp_identify_head(sp_head ** head, char * buf, uint32_t len) {
	return sp_generic_identify((void *)head, 0, buf, len);
}

int sp_mr_pack_ireq(spmr_ireq * ireq, char * buf, uint32_t len) {
	return sp_generic_pack(
		ireq, sizeof(sp_head), sizeof(spmr_ireq), buf, len);
}

int sp_mr_identify_ireq(spmr_ireq ** ireq, char * buf, uint32_t len) {
	return sp_generic_identify((void *)ireq, sizeof(sp_head), buf, len);
}

int sp_mr_pack_sreq_dreq(spmr_sreq_dreq * sreq_dreq, char * buf, uint32_t len) {
	return sp_generic_pack(
		sreq_dreq, sizeof(sp_head), sizeof(spmr_sreq_dreq), buf, len);
}

int sp_mr_identify_sreq_dreq(spmr_sreq_dreq ** sreq_dreq, char * buf, uint32_t len) {
	return sp_generic_identify((void *)sreq_dreq, sizeof(sp_head), buf, len);
}

int sp_mr_pack_srep_drep(spmr_srep_drep * srep_drep, char * buf, uint32_t len) {
	return sp_generic_pack(
		srep_drep, sizeof(sp_head), sizeof(spmr_srep_drep), buf, len);
}

int sp_mr_identify_srep_drep(spmr_srep_drep ** srep_drep, char * buf, uint32_t len) {
	return sp_generic_identify((void *)srep_drep, sizeof(sp_head), buf, len);
}

int sp_mr_pack_rrc_dreq(spmr_rrc_dreq * rrc_dreq, char * buf, uint32_t len) {
	return sp_generic_pack(
		rrc_dreq, sizeof(sp_head), sizeof(spmr_rrc_dreq), buf, len);
}

int sp_mr_identify_rrc_dreq(
	spmr_rrc_dreq ** rrc_dreq, char * buf, uint32_t len) {

	return sp_generic_identify((void *)rrc_dreq, sizeof(sp_head), buf, len);
}

int sp_mr_pack_rrc_drep(spmr_rrc_drep * rrc_drep, char * buf, uint32_t len) {
	return sp_generic_pack(
		rrc_drep, sizeof(sp_head), sizeof(spmr_rrc_drep), buf, len);
}

int sp_mr_identify_rrc_drep(
	spmr_rrc_drep ** rrc_drep, char * buf, uint32_t len) {

	return sp_generic_identify((void *)rrc_drep, sizeof(sp_head), buf, len);
}

int sp_mr_pack_rrc_ireq(spmr_rrc_ireq * rrc_ireq, char * buf, uint32_t len) {
	return sp_generic_pack(
		rrc_ireq, sizeof(sp_head), sizeof(spmr_rrc_ireq), buf, len);
}

int sp_mr_identify_rrc_ireq(
	spmr_rrc_ireq ** rrc_ireq, char * buf, uint32_t len) {

	return sp_generic_identify((void *)rrc_ireq, sizeof(sp_head), buf, len);
}
