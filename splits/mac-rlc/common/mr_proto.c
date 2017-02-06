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
 * Utilities bound to protocols.
 */

#include <string.h>

#include <mr_proto.h>

int proto_parse_data_req(
	mrp ** head,
	mrp_rreq ** req,
	char * msg,
	int len) {

	if(sizeof(mrp) + sizeof(mrp_rreq) > len) {
		return -1;
	}

	*head = (mrp *) msg;
	*req  = (mrp_rreq *) (msg + sizeof(struct mr_proto));

	return 0;
}

int proto_parse_data_req_rep(
	mrp ** head,
	mrp_rrep ** rep,
	char * msg,
	int len) {

	if(sizeof(mrp) + sizeof(mrp_rrep) > len) {
		return -1;
	}

	*head = (mrp *) msg;
	*rep  = (mrp_rrep *)(msg + sizeof(struct mr_proto));

	return 0;
}

int proto_prepare_data_req(
	uint16_t rnti,
	uint32_t frame,
	uint32_t channel,
	char * msg,
	int len) {

	mrp *      head;
	mrp_rreq * req;

	if(sizeof(mrp) + sizeof(mrp_rreq) > len) {
		return -1;
	}

	head = (mrp *)msg;

	head->type   = MR_PROTO_DATA_REQ;
	head->vers   = 1;
	head->len    = sizeof(mrp_rreq);

	req  = (mrp_rreq *) (msg + sizeof(mrp));

	req->rnti    = rnti;
	req->frame   = frame;
	req->channel = channel;

	return 0;
}

int proto_prepare_data_req_rep(
	uint16_t rnti,
	uint32_t frame,
	uint32_t channel,
	char * data,
	unsigned int datalen,
	char * buf,
	int len) {

	mrp *      head;
	mrp_rrep * rep;

	if(sizeof(mrp) + sizeof(mrp_rrep) + datalen > len) {
		return -1;
	}

	head = (mrp *)buf;

	head->type   = MR_PROTO_DATA_REQ_REP;
	head->vers   = 1;
	head->len    = sizeof(mrp_rrep) + datalen - 1;

	rep  = (mrp_rrep *) (buf + sizeof(mrp));

	rep->rnti    = rnti;
	rep->frame   = frame;
	rep->channel = channel;

	memcpy(&rep->data, data, datalen);

	return 0;
}
