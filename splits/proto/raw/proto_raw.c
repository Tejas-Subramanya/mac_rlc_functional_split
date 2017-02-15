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

int sp_mr_pack_sreq(spmr_sreq * sreq, char * buf, uint32_t len) {
	return sp_generic_pack(
		sreq, sizeof(sp_head), sizeof(spmr_sreq), buf, len);
}

int sp_mr_identify_sreq(spmr_sreq ** sreq, char * buf, uint32_t len) {
	return sp_generic_identify((void *)sreq, sizeof(sp_head), buf, len);
}

int sp_mr_pack_srep(spmr_srep * srep, char * buf, uint32_t len) {
	return sp_generic_pack(
		srep, sizeof(sp_head), sizeof(spmr_srep), buf, len);
}

int sp_mr_identify_srep(spmr_srep ** srep, char * buf, uint32_t len) {
	return sp_generic_identify((void *)srep, sizeof(sp_head), buf, len);
}

int sp_mr_pack_dreq(spmr_dreq * dreq, char * buf, uint32_t len) {
	return sp_generic_pack(
		dreq, sizeof(sp_head), sizeof(spmr_dreq), buf, len);
}

int sp_mr_identify_dreq(spmr_dreq ** dreq, char * buf, uint32_t len) {
	return sp_generic_identify((void *)dreq, sizeof(sp_head), buf, len);
}

int sp_mr_pack_drep(spmr_drep * drep, char * buf, uint32_t len) {
	return sp_generic_pack(
		drep, sizeof(sp_head), sizeof(spmr_drep), buf, len);
}

int sp_mr_identify_drep(spmr_drep ** drep, char * buf, uint32_t len) {
	return sp_generic_identify((void *)drep, sizeof(sp_head), buf, len);
}
