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
	struct mr_proto ** head,
	struct mr_proto_data_req ** req,
	char *   msg,
	int      len) {

	if(sizeof(struct mr_proto) + sizeof(struct mr_proto_data_req) > len) {
		return -1;
	}

	*head = (struct mr_proto *) msg;
	*req  = (struct mr_proto_data_req *) (msg + sizeof(struct mr_proto));

	return 0;
}

int proto_prepare_data_req(
	uint16_t rnti,
	uint32_t frame,
	uint32_t channel,
	char *   msg,
	int      len) {

	struct mr_proto *          head;
	struct mr_proto_data_req * req;

	if(sizeof(struct mr_proto) + sizeof(struct mr_proto_data_req) > len) {
		return -1;
	}

	head = (struct mr_proto *) msg;

	head->type   = MR_PROTO_DATA_REQ;
	head->vers   = 1;
	head->len    = sizeof(struct mr_proto_data_req);

	req  = (struct mr_proto_data_req *) (msg + sizeof(struct mr_proto));

	req->rnti    = rnti;
	req->frame   = frame;
	req->channel = channel;

	return 0;
}
