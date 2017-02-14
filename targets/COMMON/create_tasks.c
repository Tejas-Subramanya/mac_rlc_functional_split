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

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
# include "create_tasks.h"
# include "log.h"

# ifdef OPENAIR2
#   if defined(ENABLE_USE_MME)
#     include "sctp_eNB_task.h"
#     include "s1ap_eNB.h"
#     include "nas_ue_task.h"
#     include "udp_eNB_task.h"
#     include "gtpv1u_eNB_task.h"
#   endif
#   if ENABLE_RAL
#     include "lteRALue.h"
#     include "lteRALenb.h"
#   endif
#   include "RRC/LITE/defs.h"

#   if defined(SPLIT_MAC_RLC_CU)
#     include "split_macrlc_cu_task.h"
#     include "mr_cu.h"
#     include "rlc.h"
#   endif

#   if defined(SPLIT_MAC_RLC_DU)
#     include "split_macrlc_du_task.h"
#     include "mr_du.h"
#   endif
# endif
# include "enb_app.h"


#if defined(SPLIT_MAC_RLC_DU)

static int process_data_du(char * buf, unsigned int len) {
  sp_head * head;
  spmr_srep * status_reply = 0;
  spmr_drep * data_reply = 0;

  if(sp_identify_head(&head, buf, len)) {
    return 0;
  }

  switch(head->type) {
  /* Status reply received from CU. */
    case S_PROTO_MR_STATUS_REP:
      sp_mr_identify_srep(&status_reply, buf, len);
      memcpy(&du_status_data, status_reply, sizeof(spmr_srep));
      du_status_arrived = 1;
      break;
  /* Data_Response received from CU. */
  case S_PROTO_MR_DATA_REQ_REP:
    sp_mr_identify_drep(&data_reply, buf, len);
    memcpy(&du_data_reply, data_reply, sizeof(spmr_drep));
    du_data_arrived = 1;
    break;
  }
  return 0;
}

#endif

#if defined(SPLIT_MAC_RLC_CU) 
#define CU_BUF_SIZE 8192
char cu_outbuf[CU_BUF_SIZE] = {0};

//TO BE DONE: Correct these values
int module_idP = 1;
int eNB_index = 1;
bool enb_flagP = 1;
bool MBMS_flagP = 0;

static int process_data_cu(char *buf, unsigned int len) {
  sp_head * head;

  if(sp_identify_head(&head, buf, len)) {
    return 0;
  }

  
  int buflen;
  spmr_sreq *status_req;
  spmr_srep status_reply;
  mac_rlc_status_resp_t ret;
  
  switch(head->type) {
  /* Status req received from DU */
    case S_PROTO_MR_STATUS_REQ:

      if(sp_mr_identify_sreq(&status_req, buf, len)) {
        return -1;
      }
     
      //TO BE DONE: Define constant arguments module_idP, eNB_index, enb_flagP, MBMS_flagP before calling the function
      
      ret = mac_rlc_status_ind(module_idP, status_req->rnti, eNB_index, status_req->frame, enb_flagP, MBMS_flagP, status_req->channel, status_req->tb_size);
      status_reply.bytes = ret.bytes_in_buffer;
      status_reply.pdus = ret.pdus_in_buffer;
      status_reply.creation_frame = ret.head_sdu_creation_time;
      status_reply.remaining_bytes = ret.head_sdu_remaining_size_to_send;
      status_reply.segmented = ret.head_sdu_is_segmented;

      status_reply.rnti = status_req->rnti;
      status_reply.frame = status_req->frame;
      status_reply.channel = status_req->channel;
      status_reply.tb_size = status_req->tb_size;

      head->type = S_PROTO_MR_STATUS_REP;
      head->len = sizeof(sp_head) + sizeof(spmr_srep);

      sp_pack_head(head, cu_outbuf, CU_BUF_SIZE);
      buflen = sp_mr_pack_srep(&status_reply, cu_outbuf, CU_BUF_SIZE);      
      cu_send(cu_outbuf, buflen);
      break;

  /* Status resp received by DU */    
    case S_PROTO_MR_STATUS_REP:
      break;
  /* Data req received by DU */
    case S_PROTO_MR_DATA_REQ:
      break;
  /* Data resp received by DU */
    case S_PROTO_MR_DATA_REQ_REP:
      break;
  /* Invalid data received by DU */
    case S_PROTO_INVALID:
    default:
      printf("Type not handled; dumping data:\n");
      for(int i = 0; i < 32; i++) {
        if((i + 1) % 16 == 0) {
          printf("\n");
        }
        printf("%02x ", (unsigned char)buf[i]);
      }
      printf("\n");
      break;
  }
  return 0;
}

#endif

int create_tasks(uint32_t enb_nb, uint32_t ue_nb)
{
  itti_wait_ready(1);
  if (itti_create_task (TASK_L2L1, l2l1_task, NULL) < 0) {
    LOG_E(PDCP, "Create task for L2L1 failed\n");
    return -1;
  }

  if (enb_nb > 0) {
    /* Last task to create, others task must be ready before its start */
    if (itti_create_task (TASK_ENB_APP, eNB_app_task, NULL) < 0) {
      LOG_E(ENB_APP, "Create task for eNB APP failed\n");
      return -1;
    }
  }


# ifdef OPENAIR2
  {
#   if defined(ENABLE_USE_MME)
    {
      if (enb_nb > 0) {
        if (itti_create_task (TASK_SCTP, sctp_eNB_task, NULL) < 0) {
          LOG_E(SCTP, "Create task for SCTP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_S1AP, s1ap_eNB_task, NULL) < 0) {
          LOG_E(S1AP, "Create task for S1AP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_UDP, udp_eNB_task, NULL) < 0) {
          LOG_E(UDP_, "Create task for UDP failed\n");
          return -1;
        }

        if (itti_create_task (TASK_GTPV1_U, &gtpv1u_eNB_task, NULL) < 0) {
          LOG_E(GTPU, "Create task for GTPV1U failed\n");
          return -1;
        }
      }

#      if defined(NAS_BUILT_IN_UE)
      if (ue_nb > 0) {
        if (itti_create_task (TASK_NAS_UE, nas_ue_task, NULL) < 0) {
          LOG_E(NAS, "Create task for NAS UE failed\n");
          return -1;
        }
      }
#      endif
    }
#   endif

    if (enb_nb > 0) {
      if (itti_create_task (TASK_RRC_ENB, rrc_enb_task, NULL) < 0) {
        LOG_E(RRC, "Create task for RRC eNB failed\n");
        return -1;
      }

#   if ENABLE_RAL

      if (itti_create_task (TASK_RAL_ENB, eRAL_task, NULL) < 0) {
        LOG_E(RAL_ENB, "Create task for RAL eNB failed\n");
        return -1;
      }

#   endif
    }

    if (ue_nb > 0) {
      if (itti_create_task (TASK_RRC_UE, rrc_ue_task, NULL) < 0) {
        LOG_E(RRC, "Create task for RRC UE failed\n");
        return -1;
      }

#   if ENABLE_RAL

      if (itti_create_task (TASK_RAL_UE, mRAL_task, NULL) < 0) {
        LOG_E(RAL_UE, "Create task for RAL UE failed\n");
        return -1;
      }

#   endif
    }

#     if defined(SPLIT_MAC_RLC_CU)
    if (enb_nb > 0) {
      if (cu_init("192.168.100.100:9001", process_data_cu)) { //TO BE DONE: Change IP address
        LOG_E(SPLIT_MAC_RLC_CU, "Create thread for  SPLIT MACRLC CU failed\n");
        return -1;
         }
      }
#     endif 


#     if defined(SPLIT_MAC_RLC_DU)
    if (enb_nb > 0) {
      if (du_init("192.168.100.101:9000", process_data_du)) { //TO BE DONE: Change IP address
        LOG_E(SPLIT_MAC_RLC_DU, "Create thread for SPLIT MACRLC DU failed\n");
        return -1;
         }
      }
#     endif 
  }
# endif // openair2: NN: should be openair3


  itti_wait_ready(0);

  return 0;
}
#endif
