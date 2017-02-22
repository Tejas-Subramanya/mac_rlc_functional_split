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
mac_rlc_status_resp_t cu_status = {0};
char cu_outbuf[CU_BUF_SIZE] = {0};

int mac_rlc_cu_recv(char * buf, unsigned int len) {
  sp_head * head;

  if(sp_identify_head(&head, buf, len)) {
    return 0;
  }

  /* Send it back. */
  switch(head->type) {
    case S_PROTO_MR_STATUS_REQ:
      // To call mac_rlc_status_ind function here with the received arguments from DU
      // cu_status = mac_rlc_status_ind(received arguments);
      mac_rlc_status_reply(head, buf, len);
      break;
    default:
      cu_send(buf, len);
      /* Do nothing... */
  }
  return 0;
}

int mac_rlc_status_reply(sp_head * head, char * buf, unsigned int len) {
    int blen;
    spmr_sreq *status_request;
    spmr_srep status_reply;

    if(sp_mr_identify_sreq(&status_request, buf, len)) {
      return -1;
    }

    status_reply.rnti    = status_request->rnti;
    status_reply.frame   = status_request->frame;
    status_reply.channel = status_request->channel;
    status_reply.tb_size = status_request->tb_size;

    status_reply.bytes   = cu_status.bytes_in_buffer;
    status_reply.pdus    = cu_status.pdus_in_buffer;
    status_reply.creation_time = cu_status.head_sdu_creation_time;
    status_reply.remaining_bytes = cu_status.head_sdu_remaining_size_to_send;
    status_reply.segmented = cu_status.head_sdu_is_segmented;

    head->type    = S_PROTO_MR_STATUS_REP;
    head->len     = sizeof(sp_head) + sizeof(spmr_srep);

    sp_pack_head(head, cu_outbuf, CU_BUF_SIZE);
    blen = sp_mr_pack_srep(&status_reply, cu_outbuf, CU_BUF_SIZE);

    cu_send(cu_outbuf, blen);
    return 0;
}

#endif /* SPLIT_MAC_RLC_CU */

/*
 * This area is valid only if DU profile is enabled.
 */
#ifdef SPLIT_MAC_RLC_DU

int arrived = 1;
int du_status_arrived = 1;

int mac_rlc_du_recv(char * buf, unsigned int len) {
  
  sp_head * head;
  if(sp_identify_head(&head, buf, len)) {
    return 0;
  }
  
  switch(head->type) {
    case S_PROTO_MR_STATUS_REP:
      mr_stat_status_epilogue();
      // return the function here
      du_status_arrived = 1;
      break;
    default:
      mr_stat_ind_epilogue();
      arrived = 1;
      /* Do nothing... */
  }
  return 0;
}

#endif /* SPLIT_MAC_RLC_DU */

#endif /* SPLIT_MAC_RLC_CU || SPLIT_MAC_RLC_DU*/

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
#ifdef SPLIT_MAC_RLC_DU
  
  
  du_status_arrived = 0;
  sp_head   status_header;
  spmr_sreq status_request;
  char buf[DU_BUF_SIZE] = {0};
  int buflen = 0;
  
  status_header.type    = S_PROTO_MR_STATUS_REQ;
  status_header.vers    = 1;
  status_header.len     = sizeof(spmr_sreq);

  status_request.rnti = rntiP;
  status_request.frame = frameP;
  status_request.channel = channel_idP;
  status_request.tb_size = tb_sizeP;

  sp_pack_head(&status_header, buf, DU_BUF_SIZE);
  buflen = sp_mr_pack_sreq(&status_request, buf, DU_BUF_SIZE);
  
  mr_stat_status_prologue();
  du_send(buf, buflen);

#endif

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













