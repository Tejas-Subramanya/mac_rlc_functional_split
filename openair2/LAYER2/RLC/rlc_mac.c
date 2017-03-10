/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
                                rlc_mac.c
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
  
  CO-AUTHORS : Tejas Subramanya and Kewin Rausch
  COMPANY    : FBK CREATE-NET
  EMAIL      : t.subramanya@fbk.eu; kewin.rausch@fbk.eu 
  Description: MAC-RLC functional split
*/

//-----------------------------------------------------------------------------
#define RLC_MAC_C
#include "rlc.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/OCG/OCG_vars.h"
#include "hashtable.h"
#include "assertions.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

/******************************************************************************
 * RLC-MAC split mechanisms; only valid if split is enabled.                  *
 ******************************************************************************/

#if defined (SPLIT_MAC_RLC_CU) || defined (SPLIT_MAC_RLC_DU)


/*
 * Operations which are in common for both the profiles.
 */



/*
 * This area is valid only if CU profile is enabled.
 */
#ifdef SPLIT_MAC_RLC_CU

char cu_outbuf[CU_BUF_SIZE] = {0};
char cu_outbuf_dreq[CU_BUF_SIZE] = {0};
char cu_outbuf_rrc_dreq[CU_BUF_SIZE] = {0};

/* These globals are collected during CU local operations (the MAC is still
 * lurking underground in the CU) and used to emulate similar operations when
 * remote requests arrives.
 *
 * Actually these data are collected during status indicator request operation,
 * which is the most recurrent one.
 */

module_id_t     cu_mod_id       = 0;
eNB_index_t     cu_idx          = 0;
eNB_flag_t      cu_eNB_flag     = 1;
MBMS_flag_t     cu_MBMS_flag    = 0;

/* These are for MAC-RRC mechanisms. */
module_id_t     cu_rrc_mod_id   = 0;
eNB_flag_t      cu_rrc_eNB_flag = 1; /* Always true, probably. */
mac_enb_index_t cu_rrc_idx      = 0; /* Equal as mod_id, usually. */
uint8_t         cu_rrc_mbsfn    = 0; /* Not collected, always 0. */

/******************************************************************************
 * Reply for MAC-RLC Status Indicator.                                        *
 ******************************************************************************/

int mac_rlc_status_reply(sp_head * head, char * buf, unsigned int len) {
  int blen = 0;

  spmr_sreq * sreq = 0;
  spmr_srep srep   = {0};

  mac_rlc_status_resp_t ret  = {0};

  if(sp_mr_identify_sreq(&sreq, buf, len)) {
    return -1;
  }

  /* Replicate the normal call with given parameters: */
  ret = mac_rlc_status_ind(
    cu_mod_id,
    sreq->rnti,
    cu_idx,
    sreq->frame,
    cu_eNB_flag,
    cu_MBMS_flag,
    sreq->channel,
    sreq->tb_size);

  /* Prepare an answer: */
  srep.rnti            = sreq->rnti;
  srep.frame           = sreq->frame;
  srep.channel         = sreq->channel;
  srep.tb_size         = sreq->tb_size;
  srep.bytes           = ret.bytes_in_buffer;
  srep.pdus            = ret.pdus_in_buffer;
  srep.creation_time   = ret.head_sdu_creation_time;
  srep.remaining_bytes = ret.head_sdu_remaining_size_to_send;
  srep.segmented       = ret.head_sdu_is_segmented;

  head->type           = S_PROTO_MR_STATUS_REP;
  head->len            = sizeof(spmr_srep);

  sp_pack_head(head, cu_outbuf, CU_BUF_SIZE);
  blen = sp_mr_pack_srep(&srep, cu_outbuf, CU_BUF_SIZE);

  cu_send(cu_outbuf, blen);

  return 0;
}

/******************************************************************************
 * Reply for MAC-RLC Data Request.                                            *
 ******************************************************************************/

int mac_rlc_data_req_reply(sp_head * head, char * buf, unsigned int len) {
  int blen = 0;

  spmr_dreq * dreq = 0;
  spmr_drep   drep = {0};

  unsigned char tmpbuf[MAX_DLSCH_PAYLOAD_BYTES] = {0};
  tbs_size_t tmplen = 0;

  if(sp_mr_identify_dreq(&dreq, buf, len)) {
    return -1;
  }

  tmplen = mac_rlc_data_req(
    cu_mod_id,
    dreq->rnti,
    cu_idx,
    dreq->frame,
    cu_eNB_flag,
    cu_MBMS_flag,
    dreq->channel,
    tmpbuf);

  drep.rnti    = dreq->rnti;
  drep.frame   = dreq->frame;
  drep.channel = dreq->channel;

  head->type = S_PROTO_MR_DATA_REQ_REP;
  head->len  = sizeof(spmr_drep) + tmplen;

  sp_pack_head(head, cu_outbuf_dreq, CU_BUF_SIZE);
  blen = sp_mr_pack_drep(&drep, cu_outbuf_dreq, CU_BUF_SIZE);
  memcpy(cu_outbuf_dreq + blen, tmpbuf, tmplen);

  cu_send(cu_outbuf_dreq, blen + tmplen);

  return 0;
}

/******************************************************************************
 * Indication of MAC-RLC Data.                                                *
 ******************************************************************************/

int mac_rlc_data_ind_req(sp_head * head, char * buf, unsigned int len) {
  char * data = 0;

  spmr_ireq * ir = 0;

  if(sp_mr_identify_ireq(&ir, buf, len)) {
    return -1;
  }

  LOG_N(RLC, "-----------> [SPLIT][CU] RLC DATA_IND Len:%d, SDU:%d, Channel:%d, Frame:%d, RNTI:%x\n",
    len,
    head->len - sizeof(spmr_ireq),
    ir->channel,
    ir->frame,
    ir->rnti);

  data = buf + sizeof(sp_head) + sizeof(spmr_ireq);

  /* Just inform that the data is there. */
  mac_rlc_data_ind(
    cu_mod_id,
    ir->rnti,
    cu_idx,
    ir->frame,
    cu_eNB_flag,
    cu_MBMS_flag,
    ir->channel,
    data,
    head->len - sizeof(spmr_ireq),
    1,
    0);

  /* Data indicator does not need any feedback; is a one-way travel. */

  return 0;
}

/******************************************************************************
 * Reply for MAC-RRC Data Request.                                            *
 ******************************************************************************/

int mac_rrc_data_req_reply(sp_head * head, char * buf, unsigned int len) {
  int blen = 0;

  /* Size of this variable shall give enough room to contains a reply; look at
   * 'mac_rrc_data_req' and adapt to that buffer sizes.
   */
  char tmpbuf[CCCH_PAYLOAD_SIZE_MAX] = {0};
  int  tmplen = 0;

  spmr_rrc_dreq * dreq;
  spmr_rrc_drep drep;

  if(sp_mr_identify_rrc_dreq(&dreq, buf, len)) {
    return -1;
  }

  tmplen = mac_rrc_data_req(
    cu_rrc_mod_id,
    dreq->CC_id,
    dreq->frame,
    dreq->srb_id,
    dreq->num_tb,
    tmpbuf,
    cu_rrc_eNB_flag,
    cu_rrc_mod_id,
    cu_rrc_mbsfn);

  head->type  = S_PROTO_MR_RRC_DATA_REQ_REP;
  head->vers  = 1;
  head->len   = sizeof(spmr_rrc_drep) + tmplen;

  drep.CC_id  = dreq->CC_id;
  drep.frame  = dreq->frame;
  drep.srb_id = dreq->srb_id;

  sp_pack_head(head, cu_outbuf_rrc_dreq, CU_BUF_SIZE);
  blen = sp_mr_pack_rrc_drep(&drep, cu_outbuf_rrc_dreq, CU_BUF_SIZE);
  memcpy(buf + blen, tmpbuf, tmplen);

  cu_send(cu_outbuf_rrc_dreq, blen + tmplen);

  return 0;
}

/******************************************************************************
 * Reply for MAC-RRC Data Indicator.                                          *
 ******************************************************************************/

int mac_rrc_data_ind_reply(sp_head * head, char * buf, unsigned int len) {
  char * data = 0;

  spmr_rrc_ireq * di;

  if(sp_mr_identify_rrc_ireq(&di, buf, len)) {
    return -1;
  }

  LOG_N(RLC, "-----------> [SPLIT][CU] RRC DATA_IND Len:%d, SDU:%d, CC:%d, Frame:%d, SubF:%d, RNTI:%x, SRB:%d\n",
    len,
    head->len - sizeof(spmr_rrc_ireq),
    di->CC_id,
    di->frame,
    di->subframe,
    di->rnti,
    di->srb_id);

  data = buf + sizeof(sp_head) + sizeof(spmr_rrc_ireq);

  mac_rrc_data_ind(
    cu_rrc_mod_id,
    di->CC_id,
    di->frame,
    di->subframe,
    di->rnti,
    di->srb_id,
    data,
    head->len - sizeof(spmr_rrc_ireq),
    cu_rrc_eNB_flag,
    cu_rrc_idx,
    cu_rrc_mbsfn);

  /* Data indicator does not need any feedback; is a one-way travel. */

  return 0;
}

/******************************************************************************
 * Receiver loop in the CU.                                                   *
 ******************************************************************************/

int mac_rlc_cu_recv(char * buf, unsigned int len) {
  sp_head * head;

  if(sp_identify_head(&head, buf, len)) {
    return 0;
  }

  /* Send it back. */
  switch(head->type) {
  case S_PROTO_MR_STATUS_REQ:
    mac_rlc_status_reply(head, buf, len);
    break;
  case S_PROTO_MR_DATA_REQ:
    mac_rlc_data_req_reply(head, buf, len);
    break;
  case S_PROTO_MR_DATA_IND:
    mac_rlc_data_ind_req(head, buf, len);
    break;
  case S_PROTO_MR_RRC_DATA_REQ:
    mac_rrc_data_req_reply(head, buf, len);
    break;
  case S_PROTO_MR_RRC_DATA_IND:
    mac_rrc_data_ind_reply(head, buf, len);
    break;
  default:
    /* cu_send(buf, len);
    Do nothing... */
    break;
  }
  return 0;
}

#endif /* SPLIT_MAC_RLC_CU */

#ifdef SPLIT_MAC_RLC_DU

/******************************************************************************
 * DU-side globals.                                                           *
 ******************************************************************************/

int du_rrc_drep_ready = 0;
int du_rrc_drep_size  = 0;
char du_rrc_drep_data[DU_BUF_SIZE] = {0};

int du_rlc_srep_ready = 0;
mac_rlc_status_resp_t du_rlc_srep_empty = {0};
mac_rlc_status_resp_t du_rlc_srep_data  = {0};

int du_rlc_drep_ready = 0;
int du_rlc_drep_size  = 0;
char du_rlc_drep_data[DU_BUF_SIZE] = {0};

/******************************************************************************
 * Reply for MAC-RRC Data Indicator.                                          *
 ******************************************************************************/

int mac_rlc_handle_data_rep(sp_head * head, char * buf, unsigned int len) {
  spmr_drep * drep = 0;

  if(sp_mr_identify_drep(&drep, buf, len)) {
    return -1;
  }

  /* Fill the feedback data. */
  du_rlc_drep_size = head->len - sizeof(spmr_rrc_drep);
  memcpy(
    du_rlc_drep_data,
    buf + sizeof(sp_head) + sizeof(spmr_drep),
    du_rlc_drep_size);

  /* Allow the waiting loop to go on. */
  du_rlc_drep_ready = 1;

  return 0;
}

int mac_rlc_handle_status_rep(sp_head * head, char * buf, unsigned int len) {
  spmr_srep * srep = 0;

  if(sp_mr_identify_srep(&srep, buf, len)) {
    return -1;
  }

  /* Fill the feedback data. */
  du_rlc_srep_data.bytes_in_buffer = srep->bytes;
  du_rlc_srep_data.head_sdu_creation_time = srep->creation_time;
  du_rlc_srep_data.head_sdu_is_segmented = srep->segmented;
  du_rlc_srep_data.head_sdu_remaining_size_to_send = srep->remaining_bytes;
  du_rlc_srep_data.pdus_in_buffer = srep->pdus;

  /* Allow the waiting loop to go on. */
  du_rlc_srep_ready = 1;

  return 0;
}

int mac_rrc_handle_data_rep(sp_head * head, char * buf, unsigned int len) {
  spmr_rrc_drep * drep = 0;

  if(sp_mr_identify_rrc_drep(&drep, buf, len)) {
    return -1;
  }

  /* Fill the feedback data. */
  du_rrc_drep_size = head->len - sizeof(spmr_rrc_drep);
  memcpy(
    du_rrc_drep_data,
    buf + sizeof(sp_head) + sizeof(spmr_rrc_drep),
    du_rrc_drep_size);

  /* Allow the waiting loop to go on. */
  du_rrc_drep_ready = 1;

  return 0;
}

/******************************************************************************
 * Receiver loop in the CU.                                                   *
 ******************************************************************************/

int mac_rlc_du_recv(char * buf, unsigned int len) {
  sp_head * head = {0};

  if(sp_identify_head(&head, buf, len)) {
    return 0;
  }
  
  switch(head->type) {
    case S_PROTO_MR_STATUS_REP:
      mr_stat_status_epilogue((uint32_t)len);
      mac_rlc_handle_status_rep(head, buf, len);
      break;
    case S_PROTO_MR_DATA_REQ_REP:
      mr_stat_req_epilogue((uint32_t)len);
      mac_rlc_handle_data_rep(head, buf, len);
      break;
    case S_PROTO_MR_DATA_IND:
      mr_stat_ind_epilogue();
      break;
    case S_PROTO_MR_RRC_DATA_REQ_REP:
      mr_stat_rrc_req_epilogue((uint32_t)len);
      mac_rrc_handle_data_rep(head, buf, len);
      break;
    case S_PROTO_MR_RRC_DATA_IND:
      mr_stat_rrc_ind_epilogue();
      break;
    default:
      break;
  }

  return 0;
}

#endif /* SPLIT_MAC_RLC_DU */

#endif /* SPLIT_MAC_RLC_CU || SPLIT_MAC_RLC_DU */

/******************************************************************************
 * End of MAC-RLC split mechanisms.                                           *
 ******************************************************************************/

//#define DEBUG_MAC_INTERFACE 1

//-----------------------------------------------------------------------------
struct mac_data_ind mac_rlc_deserialize_tb (
  char     *buffer_pP,
  const tb_size_t tb_sizeP,
  num_tb_t  num_tbP,
  crc_t    *crcs_pP)
{
  //-----------------------------------------------------------------------------
  struct mac_data_ind  data_ind;
  mem_block_t*         tb_p;
  num_tb_t             nb_tb_read;
  tbs_size_t           tbs_size;

  nb_tb_read = 0;
  tbs_size   = 0;
  list_init(&data_ind.data, NULL);

  while (num_tbP > 0) {
    tb_p = get_free_mem_block(sizeof (mac_rlc_max_rx_header_size_t) + tb_sizeP);

    if (tb_p != NULL) {
      ((struct mac_tb_ind *) (tb_p->data))->first_bit = 0;
      ((struct mac_tb_ind *) (tb_p->data))->data_ptr = (uint8_t*)&tb_p->data[sizeof (mac_rlc_max_rx_header_size_t)];
      ((struct mac_tb_ind *) (tb_p->data))->size = tb_sizeP;

      if (crcs_pP) {
        ((struct mac_tb_ind *) (tb_p->data))->error_indication = crcs_pP[nb_tb_read];
      } else {
        ((struct mac_tb_ind *) (tb_p->data))->error_indication = 0;
      }

      memcpy(((struct mac_tb_ind *) (tb_p->data))->data_ptr, &buffer_pP[tbs_size], tb_sizeP);

#ifdef DEBUG_MAC_INTERFACE
      LOG_T(RLC, "[MAC-RLC] DUMP RX PDU(%d bytes):\n", tb_sizeP);
      rlc_util_print_hex_octets(RLC, ((struct mac_tb_ind *) (tb_p->data))->data_ptr, tb_sizeP);
#endif
      nb_tb_read = nb_tb_read + 1;
      tbs_size   = tbs_size   + tb_sizeP;
      list_add_tail_eurecom(tb_p, &data_ind.data);
    }

    num_tbP = num_tbP - 1;
  }

  data_ind.no_tb            = nb_tb_read;
  data_ind.tb_size          = tb_sizeP << 3;

  return data_ind;
}
//-----------------------------------------------------------------------------
tbs_size_t mac_rlc_serialize_tb (char* buffer_pP, list_t transport_blocksP)
{
  //-----------------------------------------------------------------------------
  mem_block_t *tb_p;
  tbs_size_t   tbs_size;
  tbs_size_t   tb_size;

  tbs_size = 0;

  while (transport_blocksP.nb_elements > 0) {
    tb_p = list_remove_head (&transport_blocksP);

    if (tb_p != NULL) {
      tb_size = ((struct mac_tb_req *) (tb_p->data))->tb_size;
#ifdef DEBUG_MAC_INTERFACE
      LOG_T(RLC, "[MAC-RLC] DUMP TX PDU(%d bytes):\n", tb_size);
      rlc_util_print_hex_octets(RLC, ((struct mac_tb_req *) (tb_p->data))->data_ptr, tb_size);
#endif
      memcpy(&buffer_pP[tbs_size], &((struct mac_tb_req *) (tb_p->data))->data_ptr[0], tb_size);
      tbs_size = tbs_size + tb_size;
      free_mem_block(tb_p);
    }
  }

  return tbs_size;
}
//-----------------------------------------------------------------------------
tbs_size_t mac_rlc_data_req(
  const module_id_t       module_idP,
  const rnti_t            rntiP,
  const eNB_index_t       eNB_index,
  const frame_t           frameP,
  const eNB_flag_t        enb_flagP,
  const MBMS_flag_t       MBMS_flagP,
  const logical_chan_id_t channel_idP,
  char             *buffer_pP)
{
  //-----------------------------------------------------------------------------

  struct mac_data_req    data_request;
  rb_id_t                rb_id           = 0;
  rlc_mode_t             rlc_mode        = RLC_MODE_NONE;
  rlc_mbms_id_t         *mbms_id_p       = NULL;
  rlc_union_t           *rlc_union_p     = NULL;
  hash_key_t             key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
  srb_flag_t             srb_flag        = (channel_idP <= 2) ? SRB_FLAG_YES : SRB_FLAG_NO;
  tbs_size_t             ret_tb_size         = 0;
  protocol_ctxt_t     ctxt;

/******************************************************************************
 * MAC-RLC split mechanisms.                                                  *
 ******************************************************************************/

#if defined(SPLIT_MAC_RLC_DU)
  sp_head head = {0};
  spmr_dreq dreq = {0};

  char buf[DU_BUF_SIZE] = {0};
  int buflen = 0;  
  
  head.type    = S_PROTO_MR_DATA_REQ;
  head.vers    = 1;
  head.len     = sizeof(spmr_dreq);
  
  dreq.rnti    = rntiP;
  dreq.frame   = frameP;
  dreq.channel = channel_idP;
  
  sp_pack_head(&head, buf, DU_BUF_SIZE);
  buflen = sp_mr_pack_dreq(&dreq, buf, DU_BUF_SIZE);

  if(du_send(buf, buflen)) {   
    mr_stat_req_prologue((uint32_t)buflen);
  }
#if 0
  if(du_rlc_drep_ready) {
    /* Reset of waiting condition. */
    du_rlc_drep_ready = 0;

    memcpy(buffer_pP, du_rlc_drep_data, du_rlc_drep_size);
    return du_rlc_drep_size;
  }

  return 0;
#endif
#endif

/******************************************************************************
 * End of MAC-RLC split mechanisms.                                           *
 ******************************************************************************/

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, enb_flagP, rntiP, frameP, 0,eNB_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_MAC_RLC_DATA_REQ,VCD_FUNCTION_IN);
#ifdef DEBUG_MAC_INTERFACE
  LOG_D(RLC, PROTOCOL_CTXT_FMT" MAC_RLC_DATA_REQ channel %d (%d) MAX RB %d, Num_tb %d\n",
        PROTOCOL_CTXT_ARGS((&ctxt)),
        channel_idP,
        RLC_MAX_LC,
        NB_RB_MAX);

#endif // DEBUG_MAC_INTERFACE

  if (MBMS_flagP) {
    AssertFatal (channel_idP < RLC_MAX_MBMS_LC,        "channel id is too high (%u/%d)!\n",     channel_idP, RLC_MAX_MBMS_LC);
  } else {
    AssertFatal (channel_idP < NB_RB_MAX,        "channel id is too high (%u/%d)!\n",     channel_idP, NB_RB_MAX);
  }

#ifdef OAI_EMU
  CHECK_CTXT_ARGS(&ctxt);
  //printf("MBMS_flagP %d, MBMS_FLAG_NO %d \n",MBMS_flagP, MBMS_FLAG_NO);
  //  AssertFatal (MBMS_flagP == MBMS_FLAG_NO ," MBMS FLAG SHOULD NOT BE SET IN mac_rlc_data_req in UE\n");

#endif

  if (MBMS_flagP) {
    if (enb_flagP) {
      mbms_id_p = &rlc_mbms_lcid2service_session_id_eNB[module_idP][channel_idP];
      key = RLC_COLL_KEY_MBMS_VALUE(module_idP, rntiP, enb_flagP, mbms_id_p->service_id, mbms_id_p->session_id);
    } else {
      return (tbs_size_t)0;
    }
  } else {
    if (channel_idP > 2) {
      rb_id = channel_idP - 2;
    } else {
      rb_id = channel_idP;
    }

    key = RLC_COLL_KEY_VALUE(module_idP, rntiP, enb_flagP, rb_id, srb_flag);
  }

  h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_mode = rlc_union_p->mode;
  } else {
    rlc_mode = RLC_MODE_NONE;
    AssertFatal (0 , "RLC not configured rb id %u lcid %u RNTI %x!\n", rb_id, channel_idP, rntiP);
  }

  switch (rlc_mode) {
  case RLC_MODE_NONE:
    ret_tb_size =0;
    break;

  case RLC_MODE_AM:
    data_request = rlc_am_mac_data_request(&ctxt, &rlc_union_p->rlc.am);
    ret_tb_size =mac_rlc_serialize_tb(buffer_pP, data_request.data);
    break;

  case RLC_MODE_UM:
    data_request = rlc_um_mac_data_request(&ctxt, &rlc_union_p->rlc.um);
    ret_tb_size = mac_rlc_serialize_tb(buffer_pP, data_request.data);
    break;

  case RLC_MODE_TM:
    data_request = rlc_tm_mac_data_request(&ctxt, &rlc_union_p->rlc.tm);
    ret_tb_size = mac_rlc_serialize_tb(buffer_pP, data_request.data);
    break;

  default:
    ;
  }

#if T_TRACER
  if (enb_flagP)
    T(T_ENB_RLC_MAC_DL, T_INT(module_idP), T_INT(rntiP), T_INT(channel_idP), T_INT(ret_tb_size));
#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_MAC_RLC_DATA_REQ,VCD_FUNCTION_OUT);
  return ret_tb_size;
}
//-----------------------------------------------------------------------------
void mac_rlc_data_ind     (
  const module_id_t         module_idP,
  const rnti_t              rntiP,
  const module_id_t         eNB_index,
  const frame_t             frameP,
  const eNB_flag_t          enb_flagP,
  const MBMS_flag_t         MBMS_flagP,
  const logical_chan_id_t   channel_idP,
  char                     *buffer_pP,
  const tb_size_t           tb_sizeP,
  num_tb_t                  num_tbP,
  crc_t                    *crcs_pP)
{
  //-----------------------------------------------------------------------------

  rb_id_t                rb_id      = 0;
  rlc_mode_t             rlc_mode   = RLC_MODE_NONE;
  rlc_mbms_id_t         *mbms_id_p  = NULL;
  rlc_union_t           *rlc_union_p     = NULL;
  hash_key_t             key             = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
  srb_flag_t             srb_flag        = (channel_idP <= 2) ? SRB_FLAG_YES : SRB_FLAG_NO;
  protocol_ctxt_t     ctxt;

/******************************************************************************
 * MAC-RLC split mechanisms.                                                  *
 ******************************************************************************/

#if defined(SPLIT_MAC_RLC_DU)

  sp_head head = {0};
  spmr_ireq di = {0};

  char buf[DU_BUF_SIZE]   = {0};
  int buflen = 0;  
  
  head.type  = S_PROTO_MR_DATA_IND;
  head.vers  = 1;
  head.len   = sizeof(spmr_ireq) + tb_sizeP;
  
  di.rnti    = rntiP;
  di.frame   = frameP;
  di.channel = channel_idP;
  
  sp_pack_head(&head, buf, DU_BUF_SIZE);
  buflen = sp_mr_pack_ireq(&di, buf, DU_BUF_SIZE);
  memcpy(buf + buflen, buffer_pP, tb_sizeP);

  if(du_send(buf, buflen + tb_sizeP)) {
    mr_stat_ind_prologue(buflen);
  }

  /* Returns immediately. */
 // return;

#endif

/******************************************************************************
 * End of MAC-RLC split mechanisms.                                           *
 ******************************************************************************/

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, enb_flagP, rntiP, frameP, 0, eNB_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_MAC_RLC_DATA_IND,VCD_FUNCTION_IN);
#ifdef DEBUG_MAC_INTERFACE

  if (num_tbP) {
    LOG_D(RLC, PROTOCOL_CTXT_FMT" MAC_RLC_DATA_IND on channel %d (%d), rb max %d, Num_tb %d\n",
          PROTOCOL_CTXT_ARGS(&ctxt),
          channel_idP,
          RLC_MAX_LC,
          NB_RB_MAX,
          num_tbP);
  }

#endif // DEBUG_MAC_INTERFACE
#ifdef OAI_EMU

  if (MBMS_flagP)
    AssertFatal (channel_idP < RLC_MAX_MBMS_LC,  "channel id is too high (%u/%d)!\n",
                 channel_idP, RLC_MAX_MBMS_LC);
  else
    AssertFatal (channel_idP < NB_RB_MAX,        "channel id is too high (%u/%d)!\n",
                 channel_idP, NB_RB_MAX);

  CHECK_CTXT_ARGS(&ctxt);

#endif

#if T_TRACER
  if (enb_flagP)
    T(T_ENB_RLC_MAC_UL, T_INT(module_idP), T_INT(rntiP), T_INT(channel_idP), T_INT(tb_sizeP));
#endif

  if (MBMS_flagP) {
    if (BOOL_NOT(enb_flagP)) {
      mbms_id_p = &rlc_mbms_lcid2service_session_id_ue[module_idP][channel_idP];
      key = RLC_COLL_KEY_MBMS_VALUE(module_idP, rntiP, enb_flagP, mbms_id_p->service_id, mbms_id_p->session_id);
    } else {
      return;
    }
  } else {
    if (channel_idP > 2) {
      rb_id = channel_idP - 2;
    } else {
      rb_id = channel_idP;
    }

    key = RLC_COLL_KEY_VALUE(module_idP, rntiP, enb_flagP, rb_id, srb_flag);
  }

  h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_mode = rlc_union_p->mode;
  } else {
    rlc_mode = RLC_MODE_NONE;
    //AssertFatal (0 , "%s RLC not configured rb id %u lcid %u module %u!\n", __FUNCTION__, rb_id, channel_idP, ue_module_idP);
  }

  struct mac_data_ind data_ind = mac_rlc_deserialize_tb(buffer_pP, tb_sizeP, num_tbP, crcs_pP);

  switch (rlc_mode) {
  case RLC_MODE_NONE:
    //handle_event(WARNING,"FILE %s FONCTION mac_rlc_data_ind() LINE %s : no radio bearer configured :%d\n", __FILE__, __LINE__, channel_idP);
    break;

  case RLC_MODE_AM:
    rlc_am_mac_data_indication(&ctxt, &rlc_union_p->rlc.am, data_ind);
    break;

  case RLC_MODE_UM:
    rlc_um_mac_data_indication(&ctxt, &rlc_union_p->rlc.um, data_ind);
    break;

  case RLC_MODE_TM:
    rlc_tm_mac_data_indication(&ctxt, &rlc_union_p->rlc.tm, data_ind);
    break;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_MAC_RLC_DATA_IND,VCD_FUNCTION_OUT);

}
//-----------------------------------------------------------------------------
mac_rlc_status_resp_t mac_rlc_status_ind(
  const module_id_t       module_idP,
  const rnti_t            rntiP,
  const eNB_index_t       eNB_index,
  const frame_t           frameP,
  const eNB_flag_t        enb_flagP,
  const MBMS_flag_t       MBMS_flagP,
  const logical_chan_id_t channel_idP,
  const tb_size_t         tb_sizeP)
{
  //-----------------------------------------------------------------------------
  mac_rlc_status_resp_t  mac_rlc_status_resp;
  struct mac_status_ind  tx_status;
  struct mac_status_resp status_resp;
  rb_id_t                rb_id       = 0;
  rlc_mode_t             rlc_mode    = RLC_MODE_NONE;
  rlc_mbms_id_t         *mbms_id_p   = NULL;
  rlc_union_t           *rlc_union_p = NULL;
  hash_key_t             key         = HASHTABLE_NOT_A_KEY_VALUE;
  hashtable_rc_t         h_rc;
  srb_flag_t             srb_flag    = (channel_idP <= 2) ? SRB_FLAG_YES : SRB_FLAG_NO;
  protocol_ctxt_t     ctxt;

/******************************************************************************
 * MAC-RLC split mechanisms.                                                  *
 ******************************************************************************/

#ifdef SPLIT_MAC_RLC_DU
  
  sp_head   head = {0};
  spmr_sreq sreq = {0};

  char buf[DU_BUF_SIZE] = {0};
  int buflen = 0;

  head.type    = S_PROTO_MR_STATUS_REQ;
  head.vers    = 1;
  head.len     = sizeof(spmr_sreq);

  sreq.rnti    = rntiP;
  sreq.frame   = frameP;
  sreq.channel = channel_idP;
  sreq.tb_size = tb_sizeP;

  sp_pack_head(&head, buf, DU_BUF_SIZE);
  buflen = sp_mr_pack_sreq(&sreq, buf, DU_BUF_SIZE);
  
  if(du_send(buf, buflen) > 0) {
	  mr_stat_status_prologue((uint32_t)buflen);
  }
#if 0
  if(du_rlc_srep_ready) {
    /* Reset of waiting condition. */
    du_rlc_srep_ready = 0;

    return du_rlc_srep_data;
  }

  return du_rlc_srep_empty;
#endif
#endif

#ifdef SPLIT_MAC_RLC_CU
  /*
   * Populate the globals at CU side.
   */

  if(cu_mod_id != module_idP) {
    cu_mod_id = module_idP;
    /*
     * Fill RRC stuff here too.
     */
    cu_rrc_mod_id = module_idP;
    cu_rrc_idx    = module_idP;
  }

  if(cu_idx != eNB_index) {
    cu_idx = eNB_index;
  }

  if(cu_eNB_flag != enb_flagP) {
    cu_eNB_flag     = enb_flagP;
    /*
     * Fill RRC stuff here too.
     */
    cu_rrc_eNB_flag = enb_flagP;
  }

  if(cu_MBMS_flag != MBMS_flagP) {
    cu_MBMS_flag = MBMS_flagP;
  }
#endif

/******************************************************************************
 * End of MAC-RLC split mechanisms.                                           *
 ******************************************************************************/

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, enb_flagP, rntiP, frameP, 0, eNB_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_MAC_RLC_STATUS_IND,VCD_FUNCTION_IN);
  memset (&mac_rlc_status_resp, 0, sizeof(mac_rlc_status_resp_t));
  memset (&tx_status          , 0, sizeof(struct mac_status_ind));

#ifdef OAI_EMU

  if (MBMS_flagP)
    AssertFatal (channel_idP < RLC_MAX_MBMS_LC,
                 "%s channel id is too high (%u/%d) enb module id %u ue %u!\n",
                 (enb_flagP) ? "eNB" : "UE",
                 channel_idP,
                 RLC_MAX_MBMS_LC,
                 module_idP,
                 rntiP);
  else
    AssertFatal (channel_idP < NB_RB_MAX,
                 "%s channel id is too high (%u/%d) enb module id %u ue %u!\n",
                 (enb_flagP) ? "eNB" : "UE",
                 channel_idP,
                 NB_RB_MAX,
                 module_idP,
                 rntiP);

  CHECK_CTXT_ARGS(&ctxt);

#endif



  if (MBMS_flagP) {
    if (enb_flagP) {
      mbms_id_p = &rlc_mbms_lcid2service_session_id_eNB[module_idP][channel_idP];
    } else {
      mbms_id_p = &rlc_mbms_lcid2service_session_id_ue[module_idP][channel_idP];
    }

    key = RLC_COLL_KEY_MBMS_VALUE(module_idP, rntiP, enb_flagP, mbms_id_p->service_id, mbms_id_p->session_id);
  } else {
    if (channel_idP > 2) {
      rb_id = channel_idP - 2;
    } else {
      rb_id = channel_idP;
    }

    key = RLC_COLL_KEY_VALUE(module_idP, rntiP, enb_flagP, rb_id, srb_flag);
  }

  h_rc = hashtable_get(rlc_coll_p, key, (void**)&rlc_union_p);

  if (h_rc == HASH_TABLE_OK) {
    rlc_mode = rlc_union_p->mode;
  } else {
    rlc_mode = RLC_MODE_NONE;
    //LOG_W(RLC , "[%s] RLC not configured rb id %u lcid %u module %u!\n", __FUNCTION__, rb_id, channel_idP, ue_module_idP);
    //LOG_D(RLC , "[%s] RLC not configured rb id %u lcid %u module %u!\n", __FUNCTION__, rb_id, channel_idP, ue_module_idP);
  }

  switch (rlc_mode) {
  case RLC_MODE_NONE:
    //handle_event(WARNING,"FILE %s FONCTION mac_rlc_data_ind() LINE %s : no radio bearer configured :%d\n", __FILE__, __LINE__, channel_idP);
    mac_rlc_status_resp.bytes_in_buffer                 = 0;
    break;

  case RLC_MODE_AM:
    status_resp = rlc_am_mac_status_indication(&ctxt, &rlc_union_p->rlc.am, tb_sizeP, tx_status);
    mac_rlc_status_resp.bytes_in_buffer                 = status_resp.buffer_occupancy_in_bytes;
    mac_rlc_status_resp.head_sdu_creation_time          = status_resp.head_sdu_creation_time;
    mac_rlc_status_resp.head_sdu_remaining_size_to_send = status_resp.head_sdu_remaining_size_to_send;
    mac_rlc_status_resp.head_sdu_is_segmented           = status_resp.head_sdu_is_segmented;
    //return mac_rlc_status_resp;
    break;

  case RLC_MODE_UM:
    status_resp = rlc_um_mac_status_indication(&ctxt, &rlc_union_p->rlc.um, tb_sizeP, tx_status);
    mac_rlc_status_resp.bytes_in_buffer                 = status_resp.buffer_occupancy_in_bytes;
    mac_rlc_status_resp.pdus_in_buffer                  = status_resp.buffer_occupancy_in_pdus;
    mac_rlc_status_resp.head_sdu_creation_time          = status_resp.head_sdu_creation_time;
    mac_rlc_status_resp.head_sdu_remaining_size_to_send = status_resp.head_sdu_remaining_size_to_send;
    mac_rlc_status_resp.head_sdu_is_segmented           = status_resp.head_sdu_is_segmented;
    //   return mac_rlc_status_resp;
    break;

  case RLC_MODE_TM:
    status_resp = rlc_tm_mac_status_indication(&ctxt, &rlc_union_p->rlc.tm, tb_sizeP, tx_status);
    mac_rlc_status_resp.bytes_in_buffer = status_resp.buffer_occupancy_in_bytes;
    mac_rlc_status_resp.pdus_in_buffer  = status_resp.buffer_occupancy_in_pdus;
    // return mac_rlc_status_resp;
    break;

  default:
    mac_rlc_status_resp.bytes_in_buffer                 = 0 ;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_MAC_RLC_STATUS_IND,VCD_FUNCTION_OUT);
  
  return mac_rlc_status_resp;
}













