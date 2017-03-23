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

/*! \file l2_interface.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author Raymond Knopp and Navid Nikaein
 * \date 2010-2014
 * \version 1.0
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr

  CO-AUTHORS : Tejas Subramanya and Kewin Rausch
  COMPANY    : FBK CREATE-NET
  EMAIL      : t.subramanya@fbk.eu; kewin.rausch@fbk.eu 
  Description: MAC-RLC functional split
 */

#include "platform_types.h"
//#include "openair_defs.h"
//#include "openair_proto.h"
#include "defs.h"
#include "extern.h"
//#include "mac_lchan_interface.h"
//#include "openair_rrc_utils.h"
//#include "openair_rrc_main.h"
#include "UTIL/LOG/log.h"
#include "rrc_eNB_UE_context.h"
#include "pdcp.h"
#include "msc.h"
#include "create_tasks.h"

#ifdef PHY_EMUL
#include "SIMULATION/simulation_defs.h"
extern EMULATION_VARS *Emul_vars;
extern eNB_MAC_INST *eNB_mac_inst;
extern UE_MAC_INST *UE_mac_inst;
#endif

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

//#define RRC_DATA_REQ_DEBUG
#define DEBUG_RRC 1

#    ifdef SPLIT_MAC_RLC_CU
#        include "mr_cu.h"
#    endif /* SPLIT_MAC_RLC_CU */

#    ifdef SPLIT_MAC_RLC_DU
#        include "mr_du.h"
#    endif /* SPLIT_MAC_RLC_DU */

mui_t mui=0;

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_req(
  const module_id_t Mod_idP,
  const int         CC_id,
  const frame_t     frameP,
  const rb_id_t     Srb_id,
  const uint8_t     Nb_tb,
  uint8_t*    const buffer_pP,
  const eNB_flag_t  enb_flagP,
  const uint8_t     eNB_index,
  const uint8_t     mbsfn_sync_area
)
//--------------------------------------------------------------------------
{
  SRB_INFO *Srb_info;
  uint8_t Sdu_size=0;

/******************************************************************************
 * MAC-RLC split mechanisms.                                                  *
 ******************************************************************************/

#if defined(SPLIT_MAC_RLC_DU)
  sp_head head;
  spmr_rrc_dreq dreq;
  char buf[DU_BUF_SIZE] = {0};
  int buflen = 0;  
  int sdu_size = 0;

  head.type = S_PROTO_MR_RRC_DATA_REQ;
  head.vers = 1;
  head.len  = sizeof(spmr_rrc_dreq);
  
  dreq.CC_id       = CC_id;
  dreq.frame       = frameP;
  dreq.srb_id      = Srb_id;
  dreq.num_tb      = Nb_tb;
  
  sp_pack_head(&head, buf, DU_BUF_SIZE);
  buflen = sp_mr_pack_rrc_dreq(&dreq, buf, DU_BUF_SIZE);

  if(du_send(buf, buflen) > 0) {
    mr_stat_rrc_req_prologue((uint32_t)buflen);
  }

  if((Srb_id & RAB_OFFSET) == BCCH) {
    if((frameP%2) == 0) {
      pthread_spin_lock(&du_lock);
      memcpy(
            buffer_pP,
            du_rrc_drep_data_SIB1,
            du_rrc_drep_size_SIB1);
      sdu_size = du_rrc_drep_size_SIB1;
      pthread_spin_unlock(&du_lock);
      return sdu_size;
    }
    if(frameP%8 == 1) {
      pthread_spin_lock(&du_lock);
      memcpy(
            buffer_pP,
            du_rrc_drep_data_SIB23,
            du_rrc_drep_size_SIB23);
      sdu_size = du_rrc_drep_size_SIB23;
      pthread_spin_unlock(&du_lock);
      return sdu_size;
    }
  }
  if((Srb_id & RAB_OFFSET) == CCCH) {
    pthread_spin_lock(&du_lock);
    memcpy(
          buffer_pP,
          du_rrc_drep_data_SRB0,
          du_rrc_drep_size_SRB0);
    sdu_size = du_rrc_drep_size_SRB0;
    pthread_spin_unlock(&du_lock);
    return sdu_size;
  }
    
  return 0;

//#endif
#endif

/******************************************************************************
 * End of MAC-RLC split mechanisms.                                           *
 ******************************************************************************/

#ifdef DEBUG_RRC
  int i;
  LOG_T(RRC,"[eNB %d] mac_rrc_data_req to SRB ID=%d\n",Mod_idP,Srb_id);
#endif

  if( enb_flagP == ENB_FLAG_YES) {

    if((Srb_id & RAB_OFFSET) == BCCH) {
      if(eNB_rrc_inst[Mod_idP].carrier[CC_id].SI.Active==0) {
        return 0;
      }

      // All even frames transmit SIB in SF 5
      if (eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB1 == 255) {
        LOG_E(RRC,"[eNB %d] MAC Request for SIB1 and SIB1 not initialized\n",Mod_idP);
        mac_xface->macphy_exit("mac_rrc_data_req:  MAC Request for SIB1 and SIB1 not initialized");
      }

      if ((frameP%2) == 0) {
        memcpy(&buffer_pP[0],
               eNB_rrc_inst[Mod_idP].carrier[CC_id].SIB1,
               eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB1);

#if defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int sib1_size = eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB1;
          int sdu_size = sizeof(RRC_MAC_BCCH_DATA_REQ (message_p).sdu);

          if (sib1_size > sdu_size) {
            LOG_E(RRC, "SIB1 SDU larger than BCCH SDU buffer size (%d, %d)", sib1_size, sdu_size);
            sib1_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_BCCH_DATA_REQ);
          RRC_MAC_BCCH_DATA_REQ (message_p).frame    = frameP;
          RRC_MAC_BCCH_DATA_REQ (message_p).sdu_size = sib1_size;
          memset (RRC_MAC_BCCH_DATA_REQ (message_p).sdu, 0, BCCH_SDU_SIZE);
          memcpy (RRC_MAC_BCCH_DATA_REQ (message_p).sdu,
                  eNB_rrc_inst[Mod_idP].carrier[CC_id].SIB1,
                  sib1_size);
          RRC_MAC_BCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

#ifdef DEBUG_RRC
        LOG_T(RRC,"[eNB %d] Frame %d : BCCH request => SIB 1\n",Mod_idP,frameP);

        for (i=0; i<eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB1; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
#endif
        return (eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB1);
      } // All RFN mod 8 transmit SIB2-3 in SF 5
      else if ((frameP%8) == 1) {
        memcpy(&buffer_pP[0],
               eNB_rrc_inst[Mod_idP].carrier[CC_id].SIB23,
               eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB23);

#if defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int sib23_size = eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB23;
          int sdu_size = sizeof(RRC_MAC_BCCH_DATA_REQ (message_p).sdu);

          if (sib23_size > sdu_size) {
            LOG_E(RRC, "SIB23 SDU larger than BCCH SDU buffer size (%d, %d)", sib23_size, sdu_size);
            sib23_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_BCCH_DATA_REQ);
          RRC_MAC_BCCH_DATA_REQ (message_p).frame = frameP;
          RRC_MAC_BCCH_DATA_REQ (message_p).sdu_size = sib23_size;
          memset (RRC_MAC_BCCH_DATA_REQ (message_p).sdu, 0, BCCH_SDU_SIZE);
          memcpy (RRC_MAC_BCCH_DATA_REQ (message_p).sdu,
                  eNB_rrc_inst[Mod_idP].carrier[CC_id].SIB23,
                  sib23_size);
          RRC_MAC_BCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

#ifdef DEBUG_RRC
        LOG_T(RRC,"[eNB %d] Frame %d BCCH request => SIB 2-3\n",Mod_idP,frameP);

        for (i=0; i<eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB23; i++) {
          LOG_T(RRC,"%x.",buffer_pP[i]);
        }

        LOG_T(RRC,"\n");
#endif
        return(eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_SIB23);
      } else {
        return(0);
      }
    }

    if( (Srb_id & RAB_OFFSET ) == CCCH) {
      LOG_T(RRC,"[eNB %d] Frame %d CCCH request (Srb_id %d)\n",Mod_idP,frameP, Srb_id);

      if(eNB_rrc_inst[Mod_idP].carrier[CC_id].Srb0.Active==0) {
        LOG_E(RRC,"[eNB %d] CCCH Not active\n",Mod_idP);
        return -1;
      }

      Srb_info=&eNB_rrc_inst[Mod_idP].carrier[CC_id].Srb0;

      // check if data is there for MAC
      if(Srb_info->Tx_buffer.payload_size>0) { //Fill buffer
        LOG_D(RRC,"[eNB %d] CCCH (%p) has %d bytes (dest: %p, src %p)\n",Mod_idP,Srb_info,Srb_info->Tx_buffer.payload_size,buffer_pP,Srb_info->Tx_buffer.Payload);

#if defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int ccch_size = Srb_info->Tx_buffer.payload_size;
          int sdu_size = sizeof(RRC_MAC_CCCH_DATA_REQ (message_p).sdu);

          if (ccch_size > sdu_size) {
            LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", ccch_size, sdu_size);
            ccch_size = sdu_size;
          }

          message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_CCCH_DATA_REQ);
          RRC_MAC_CCCH_DATA_REQ (message_p).frame = frameP;
          RRC_MAC_CCCH_DATA_REQ (message_p).sdu_size = ccch_size;
          memset (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, 0, CCCH_SDU_SIZE);
          memcpy (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, Srb_info->Tx_buffer.Payload, ccch_size);
          RRC_MAC_CCCH_DATA_REQ (message_p).enb_index = eNB_index;

          itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
        }
#endif

        memcpy(buffer_pP,Srb_info->Tx_buffer.Payload,Srb_info->Tx_buffer.payload_size);
        Sdu_size = Srb_info->Tx_buffer.payload_size;
        Srb_info->Tx_buffer.payload_size=0;
      }

      return (Sdu_size);
    }

#ifdef Rel10

    if((Srb_id & RAB_OFFSET) == MCCH) {
      if(eNB_rrc_inst[Mod_idP].carrier[CC_id].MCCH_MESS[mbsfn_sync_area].Active==0) {
        return 0;  // this parameter is set in function init_mcch in rrc_eNB.c
      }

      // this part not needed as it is done in init_mcch
      /*     if (eNB_rrc_inst[Mod_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area] == 255) {
      LOG_E(RRC,"[eNB %d] MAC Request for MCCH MESSAGE and MCCH MESSAGE is not initialized\n",Mod_id);
      mac_xface->macphy_exit("");
      }*/


#if defined(ENABLE_ITTI)
      {
        MessageDef *message_p;
        int mcch_size = eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area];
        int sdu_size = sizeof(RRC_MAC_MCCH_DATA_REQ (message_p).sdu);

        if (mcch_size > sdu_size) {
          LOG_E(RRC, "SDU larger than MCCH SDU buffer size (%d, %d)", mcch_size, sdu_size);
          mcch_size = sdu_size;
        }

        message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_MAC_MCCH_DATA_REQ);
        RRC_MAC_MCCH_DATA_REQ (message_p).frame = frameP;
        RRC_MAC_MCCH_DATA_REQ (message_p).sdu_size = mcch_size;
        memset (RRC_MAC_MCCH_DATA_REQ (message_p).sdu, 0, MCCH_SDU_SIZE);
        memcpy (RRC_MAC_MCCH_DATA_REQ (message_p).sdu,
                eNB_rrc_inst[Mod_idP].carrier[CC_id].MCCH_MESSAGE[mbsfn_sync_area],
                mcch_size);
        RRC_MAC_MCCH_DATA_REQ (message_p).enb_index = eNB_index;
        RRC_MAC_MCCH_DATA_REQ (message_p).mbsfn_sync_area = mbsfn_sync_area;

        itti_send_msg_to_task (TASK_MAC_ENB, ENB_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
      }
#endif

      memcpy(&buffer_pP[0],
             eNB_rrc_inst[Mod_idP].carrier[CC_id].MCCH_MESSAGE[mbsfn_sync_area],
             eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]);

#ifdef DEBUG_RRC
      LOG_D(RRC,"[eNB %d] Frame %d : MCCH request => MCCH_MESSAGE \n",Mod_idP,frameP);

      for (i=0; i<eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]; i++) {
        LOG_T(RRC,"%x.",buffer_pP[i]);
      }

      LOG_T(RRC,"\n");
#endif

      return (eNB_rrc_inst[Mod_idP].carrier[CC_id].sizeof_MCCH_MESSAGE[mbsfn_sync_area]);
      //      }
      //else
      //return(0);
    }

#endif //Rel10
  } else {  //This is an UE

    LOG_D(RRC,"[UE %d] Frame %d Filling CCCH SRB_ID %d\n",Mod_idP,frameP,Srb_id);
    LOG_D(RRC,"[UE %d] Frame %d buffer_pP status %d,\n",Mod_idP,frameP, UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size);


    if( (UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size > 0) ) {

#if defined(ENABLE_ITTI)
      {
        MessageDef *message_p;
        int ccch_size = UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size;
        int sdu_size = sizeof(RRC_MAC_CCCH_DATA_REQ (message_p).sdu);

        if (ccch_size > sdu_size) {
          LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", ccch_size, sdu_size);
          ccch_size = sdu_size;
        }

        message_p = itti_alloc_new_message (TASK_RRC_UE, RRC_MAC_CCCH_DATA_REQ);
        RRC_MAC_CCCH_DATA_REQ (message_p).frame = frameP;
        RRC_MAC_CCCH_DATA_REQ (message_p).sdu_size = ccch_size;
        memset (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, 0, CCCH_SDU_SIZE);
        memcpy (RRC_MAC_CCCH_DATA_REQ (message_p).sdu, UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.Payload, ccch_size);
        RRC_MAC_CCCH_DATA_REQ (message_p).enb_index = eNB_index;

        itti_send_msg_to_task (TASK_MAC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
      }
#endif

      memcpy(&buffer_pP[0],&UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.Payload[0],UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size);
      uint8_t Ret_size=UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size;
      //   UE_rrc_inst[Mod_id].Srb0[eNB_index].Tx_buffer.payload_size=0;
      UE_rrc_inst[Mod_idP].Info[eNB_index].T300_active = 1;
      UE_rrc_inst[Mod_idP].Info[eNB_index].T300_cnt = 0;
      //      msg("[RRC][UE %d] Sending rach\n",Mod_id);
      return(Ret_size);
    } else {
      return 0;
    }
  }

  return(0);
}

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_ind(
  const module_id_t     module_idP,
  const int             CC_id,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t*        sduP,
  const sdu_size_t      sdu_lenP,
  const eNB_flag_t      eNB_flagP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t         mbsfn_sync_areaP
)
//--------------------------------------------------------------------------
{
  SRB_INFO *Srb_info;
  protocol_ctxt_t ctxt;
  sdu_size_t      sdu_size = 0;

  /* for no gcc warnings */
  (void)sdu_size;

/******************************************************************************
 * MAC-RLC split mechanisms.                                                  *
 ******************************************************************************/

#if defined(SPLIT_MAC_RLC_DU)

  sp_head head;
  spmr_rrc_ireq ireq;
  char buf[DU_BUF_SIZE] = {0};
  int buflen = 0;  
  
  head.type     = S_PROTO_MR_RRC_DATA_IND;
  head.vers     = 1;
  head.len      = sizeof(spmr_rrc_ireq) + sdu_lenP;
  
  ireq.CC_id    = CC_id;
  ireq.rnti     = rntiP;
  ireq.frame    = frameP;
  ireq.subframe = sub_frameP;
  ireq.srb_id   = srb_idP;
  
  sp_pack_head(&head, buf, DU_BUF_SIZE);
  buflen = sp_mr_pack_rrc_ireq(&ireq, buf, DU_BUF_SIZE);
  memcpy(buf + buflen, sduP, sdu_lenP);

  if(du_send(buf, buflen + sdu_lenP) > 0) {
    mr_stat_rrc_ind_prologue(buflen);
  }

  /* Send the message and return immediately. Local RRC is not required. */
  return 0;

#endif

/******************************************************************************
 * End of MAC-RLC split mechanisms.                                           *
 ******************************************************************************/

  /*
  int si_window;
   */
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, eNB_flagP, rntiP, frameP, sub_frameP,eNB_indexP);

  if(eNB_flagP == ENB_FLAG_NO) {
    if(srb_idP == BCCH) {
      LOG_D(RRC,"[UE %d] Received SDU for BCCH on SRB %d from eNB %d\n",module_idP,srb_idP,eNB_indexP);

#if defined(ENABLE_ITTI)
      {
        MessageDef *message_p;
        int msg_sdu_size = sizeof(RRC_MAC_BCCH_DATA_IND (message_p).sdu);

        if (sdu_lenP > msg_sdu_size) {
          LOG_E(RRC, "SDU larger than BCCH SDU buffer size (%d, %d)", sdu_lenP, msg_sdu_size);
          sdu_size = msg_sdu_size;
        } else {
          sdu_size = sdu_lenP;
        }

        message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_BCCH_DATA_IND);
        memset (RRC_MAC_BCCH_DATA_IND (message_p).sdu, 0, BCCH_SDU_SIZE);
        RRC_MAC_BCCH_DATA_IND (message_p).frame     = frameP;
        RRC_MAC_BCCH_DATA_IND (message_p).sub_frame = sub_frameP;
        RRC_MAC_BCCH_DATA_IND (message_p).sdu_size  = sdu_size;
        memcpy (RRC_MAC_BCCH_DATA_IND (message_p).sdu, sduP, sdu_size);
        RRC_MAC_BCCH_DATA_IND (message_p).enb_index = eNB_indexP;
        RRC_MAC_BCCH_DATA_IND (message_p).rsrq      = 30 /* TODO change phy to report rspq */;
        RRC_MAC_BCCH_DATA_IND (message_p).rsrp      = 45 /* TODO change phy to report rspp */;

        itti_send_msg_to_task (TASK_RRC_UE, ctxt.instance, message_p);
      }
#else
      decode_BCCH_DLSCH_Message(&ctxt,eNB_indexP,(uint8_t*)sduP,sdu_lenP, 0, 0);
#endif
    }

    if(srb_idP == PCCH) {
      LOG_D(RRC,"[UE %d] Received SDU for PCCH on SRB %d from eNB %d\n",module_idP,srb_idP,eNB_indexP);
      decode_PCCH_DLSCH_Message(&ctxt,eNB_indexP,(uint8_t*)sduP,sdu_lenP);
    }
    if((srb_idP & RAB_OFFSET) == CCCH) {
      if (sdu_lenP>0) {
        LOG_T(RRC,"[UE %d] Received SDU for CCCH on SRB %d from eNB %d\n",module_idP,srb_idP & RAB_OFFSET,eNB_indexP);

#if defined(ENABLE_ITTI)
        {
          MessageDef *message_p;
          int msg_sdu_size = CCCH_SDU_SIZE;

          if (sdu_lenP > msg_sdu_size) {
            LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
            sdu_size = msg_sdu_size;
          } else {
            sdu_size =  sdu_lenP;
          }

          message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_CCCH_DATA_IND);
          memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
          memcpy (RRC_MAC_CCCH_DATA_IND (message_p).sdu, sduP, sdu_size);
          RRC_MAC_CCCH_DATA_IND (message_p).frame     = frameP;
          RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = sub_frameP;
          RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = sdu_size;
          RRC_MAC_CCCH_DATA_IND (message_p).enb_index = eNB_indexP;
          RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rntiP;
          itti_send_msg_to_task (TASK_RRC_UE, ctxt.instance, message_p);
        }
#else
        Srb_info = &UE_rrc_inst[module_idP].Srb0[eNB_indexP];
        memcpy(Srb_info->Rx_buffer.Payload,sduP,sdu_lenP);
        Srb_info->Rx_buffer.payload_size = sdu_lenP;
        rrc_ue_decode_ccch(&ctxt, Srb_info, eNB_indexP);
#endif
      }
    }

#ifdef Rel10

    if ((srb_idP & RAB_OFFSET) == MCCH) {
      LOG_T(RRC,"[UE %d] Frame %d: Received SDU on MBSFN sync area %d for MCCH on SRB %d from eNB %d\n",
            module_idP,frameP, mbsfn_sync_areaP, srb_idP & RAB_OFFSET,eNB_indexP);

#if defined(ENABLE_ITTI)
      {
        MessageDef *message_p;
        int msg_sdu_size = sizeof(RRC_MAC_MCCH_DATA_IND (message_p).sdu);

        if (sdu_size > msg_sdu_size) {
          LOG_E(RRC, "SDU larger than MCCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
          sdu_size = msg_sdu_size;
        }

        message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_MCCH_DATA_IND);
        RRC_MAC_MCCH_DATA_IND (message_p).frame           = frameP;
        RRC_MAC_MCCH_DATA_IND (message_p).sub_frame       = sub_frameP;
        RRC_MAC_MCCH_DATA_IND (message_p).sdu_size        = sdu_lenP;
        memset (RRC_MAC_MCCH_DATA_IND (message_p).sdu, 0, MCCH_SDU_SIZE);
        memcpy (RRC_MAC_MCCH_DATA_IND (message_p).sdu, sduP, sdu_lenP);
        RRC_MAC_MCCH_DATA_IND (message_p).enb_index       = eNB_indexP;
        RRC_MAC_MCCH_DATA_IND (message_p).mbsfn_sync_area = mbsfn_sync_areaP;
        itti_send_msg_to_task (TASK_RRC_UE, ctxt.instance, message_p);
      }
#else
      decode_MCCH_Message(&ctxt, eNB_indexP, sduP, sdu_lenP, mbsfn_sync_areaP);
#endif
    }

#endif // Rel10

  } else { // This is an eNB
    Srb_info = &eNB_rrc_inst[module_idP].carrier[CC_id].Srb0;
    LOG_T(RRC,"[eNB %d] Received SDU for CCCH on SRB %d\n",module_idP,Srb_info->Srb_id);

#if defined(ENABLE_ITTI)
    {
      MessageDef *message_p;
      int msg_sdu_size = sizeof(RRC_MAC_CCCH_DATA_IND (message_p).sdu);

      if (sdu_lenP > msg_sdu_size) {
        LOG_E(RRC, "SDU larger than CCCH SDU buffer size (%d, %d)", sdu_lenP, msg_sdu_size);
        sdu_size = msg_sdu_size;
      } else {
        sdu_size = sdu_lenP;
      }

      message_p = itti_alloc_new_message (TASK_MAC_ENB, RRC_MAC_CCCH_DATA_IND);
      RRC_MAC_CCCH_DATA_IND (message_p).frame     = frameP;
      RRC_MAC_CCCH_DATA_IND (message_p).sub_frame = sub_frameP;
      RRC_MAC_CCCH_DATA_IND (message_p).rnti      = rntiP;
      RRC_MAC_CCCH_DATA_IND (message_p).sdu_size  = sdu_size;
      RRC_MAC_CCCH_DATA_IND (message_p).CC_id = CC_id;
      memset (RRC_MAC_CCCH_DATA_IND (message_p).sdu, 0, CCCH_SDU_SIZE);
      memcpy (RRC_MAC_CCCH_DATA_IND (message_p).sdu, sduP, sdu_size);
      itti_send_msg_to_task (TASK_RRC_ENB, ctxt.instance, message_p);
    }
#else

    //    msg("\n******INST %d Srb_info %p, Srb_id=%d****\n\n",Mod_id,Srb_info,Srb_info->Srb_id);
    if (sdu_lenP > 0) {
      memcpy(Srb_info->Rx_buffer.Payload,sduP,sdu_lenP);
      Srb_info->Rx_buffer.payload_size = sdu_lenP;
      rrc_eNB_decode_ccch(&ctxt, Srb_info, CC_id);
    }

#endif
  }

  return(0);

}

//-------------------------------------------------------------------------------------------//
// this function is Not USED anymore
void mac_sync_ind(module_id_t Mod_idP,uint8_t Status)
{
  //-------------------------------------------------------------------------------------------//
}

//------------------------------------------------------------------------------
uint8_t
rrc_data_req(
  const protocol_ctxt_t*   const ctxt_pP,
  const rb_id_t                  rb_idP,
  const mui_t                    muiP,
  const confirm_t                confirmP,
  const sdu_size_t               sdu_sizeP,
  uint8_t*                 const buffer_pP,
  const pdcp_transmission_mode_t modeP
)
//------------------------------------------------------------------------------
{
  MSC_LOG_TX_MESSAGE(
    ctxt_pP->enb_flag ? MSC_RRC_ENB : MSC_RRC_UE,
    ctxt_pP->enb_flag ? MSC_PDCP_ENB : MSC_PDCP_UE,
    buffer_pP,
    sdu_sizeP,
    MSC_AS_TIME_FMT"RRC_DCCH_DATA_REQ UE %x MUI %d size %u",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ctxt_pP->rnti,
    muiP,
    sdu_sizeP);

#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
    uint8_t *message_buffer;

    message_buffer = itti_malloc (
                       ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE,
                       ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
                       sdu_sizeP);

    memcpy (message_buffer, buffer_pP, sdu_sizeP);

    message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, RRC_DCCH_DATA_REQ);
    RRC_DCCH_DATA_REQ (message_p).frame     = ctxt_pP->frame;
    RRC_DCCH_DATA_REQ (message_p).enb_flag  = ctxt_pP->enb_flag;
    RRC_DCCH_DATA_REQ (message_p).rb_id     = rb_idP;
    RRC_DCCH_DATA_REQ (message_p).muip      = muiP;
    RRC_DCCH_DATA_REQ (message_p).confirmp  = confirmP;
    RRC_DCCH_DATA_REQ (message_p).sdu_size  = sdu_sizeP;
    RRC_DCCH_DATA_REQ (message_p).sdu_p     = message_buffer;
    RRC_DCCH_DATA_REQ (message_p).mode      = modeP;
    RRC_DCCH_DATA_REQ (message_p).module_id = ctxt_pP->module_id;
    RRC_DCCH_DATA_REQ (message_p).rnti      = ctxt_pP->rnti;
    RRC_DCCH_DATA_REQ (message_p).eNB_index = ctxt_pP->eNB_index;

    itti_send_msg_to_task (
      ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE,
      ctxt_pP->instance,
      message_p);
    return TRUE; // TODO should be changed to a CNF message later, currently RRC lite does not used the returned value anyway.

  }
#else
  return pdcp_data_req (
           ctxt_pP,
           SRB_FLAG_YES,
           rb_idP,
           muiP,
           confirmP,
           sdu_sizeP,
           buffer_pP,
           modeP);
#endif
}

//------------------------------------------------------------------------------
void
rrc_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                Srb_id,
  const sdu_size_t             sdu_sizeP,
  const uint8_t*   const       buffer_pP
)
//------------------------------------------------------------------------------
{
  rb_id_t    DCCH_index = Srb_id;

  if (ctxt_pP->enb_flag == ENB_FLAG_NO) {
    LOG_N(RRC, "[UE %x] Frame %d: received a DCCH %d message on SRB %d with Size %d from eNB %d\n",
          ctxt_pP->module_id, ctxt_pP->frame, DCCH_index,Srb_id,sdu_sizeP,  ctxt_pP->eNB_index);
  } else {
    LOG_N(RRC, "[eNB %d] Frame %d: received a DCCH %d message on SRB %d with Size %d from UE %x\n",
          ctxt_pP->module_id,
          ctxt_pP->frame,
          DCCH_index,
          Srb_id,
          sdu_sizeP,
          ctxt_pP->rnti);
  }

#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    // Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling).
    uint8_t *message_buffer;

    message_buffer = itti_malloc (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, sdu_sizeP);
    memcpy (message_buffer, buffer_pP, sdu_sizeP);

    message_p = itti_alloc_new_message (ctxt_pP->enb_flag ? TASK_PDCP_ENB : TASK_PDCP_UE, RRC_DCCH_DATA_IND);
    RRC_DCCH_DATA_IND (message_p).frame      = ctxt_pP->frame;
    RRC_DCCH_DATA_IND (message_p).dcch_index = DCCH_index;
    RRC_DCCH_DATA_IND (message_p).sdu_size   = sdu_sizeP;
    RRC_DCCH_DATA_IND (message_p).sdu_p      = message_buffer;
    RRC_DCCH_DATA_IND (message_p).rnti       = ctxt_pP->rnti;
    RRC_DCCH_DATA_IND (message_p).module_id  = ctxt_pP->module_id;
    RRC_DCCH_DATA_IND (message_p).eNB_index  = ctxt_pP->eNB_index;

    itti_send_msg_to_task (ctxt_pP->enb_flag ? TASK_RRC_ENB : TASK_RRC_UE, ctxt_pP->instance, message_p);
  }
#else

  if (ctxt_pP->enb_flag == ENB_FLAG_YES) {
    rrc_eNB_decode_dcch(
      ctxt_pP,
      DCCH_index,
      buffer_pP,
      sdu_sizeP);
  } else {
//#warning "LG put 0 to arg4 that is eNB index"
    rrc_ue_decode_dcch(
      ctxt_pP,
      DCCH_index,
      buffer_pP,
      0);
  }

#endif
}

//-------------------------------------------------------------------------------------------//
void rrc_in_sync_ind(module_id_t Mod_idP, frame_t frameP, uint16_t eNB_index)
{
  //-------------------------------------------------------------------------------------------//
#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;
    //LOG_I(RRC,"sending a message to task_mac_ue\n");
    message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_IN_SYNC_IND);
    RRC_MAC_IN_SYNC_IND (message_p).frame = frameP;
    RRC_MAC_IN_SYNC_IND (message_p).enb_index = eNB_index;

    itti_send_msg_to_task (TASK_RRC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
  }
#else
  UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt=0;

  if (UE_rrc_inst[Mod_idP].Info[eNB_index].T310_active==1) {
    UE_rrc_inst[Mod_idP].Info[eNB_index].N311_cnt++;
  }

#endif
}

//-------------------------------------------------------------------------------------------//
void rrc_out_of_sync_ind(module_id_t Mod_idP, frame_t frameP, uint16_t eNB_index)
{
  //-------------------------------------------------------------------------------------------//
  if (UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt>10)
    LOG_I(RRC,"[UE %d] Frame %d: OUT OF SYNC FROM eNB %d (T310 active %d : T310 %d, N310 %d, N311 %d)\n ",
	  Mod_idP,frameP,eNB_index,
	  UE_rrc_inst[Mod_idP].Info[eNB_index].T300_active,
	  UE_rrc_inst[Mod_idP].Info[eNB_index].T310_cnt,
	  UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt,
	  UE_rrc_inst[Mod_idP].Info[eNB_index].N311_cnt);
  
#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;

    message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_OUT_OF_SYNC_IND);
    RRC_MAC_OUT_OF_SYNC_IND (message_p).frame = frameP;
    RRC_MAC_OUT_OF_SYNC_IND (message_p).enb_index = eNB_index;

    itti_send_msg_to_task (TASK_RRC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
  }
#else
  UE_rrc_inst[Mod_idP].Info[eNB_index].N310_cnt++;
#endif
}

//------------------------------------------------------------------------------
int
mac_eNB_get_rrc_status(
  const module_id_t Mod_idP,
  const rnti_t      rntiP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   &eNB_rrc_inst[Mod_idP],
                   rntiP);

  if (ue_context_p != NULL) {
    return(ue_context_p->ue_context.Status);
  } else {
    return RRC_INACTIVE;
  }
}

//------------------------------------------------------------------------------
int
mac_eNB_get_rrc_measGap_offset(
  const module_id_t module_idP,
  const rnti_t      rntiP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[module_idP], rntiP);

  if (ue_context_p != NULL && ue_context_p->ue_context.measGapConfig != NULL &&
        ue_context_p->ue_context.measGapConfig->present ==
                                                      MeasGapConfig_PR_setup) {
    if (ue_context_p->ue_context.measGapConfig->
                choice.setup.gapOffset.present ==
                                      MeasGapConfig__setup__gapOffset_PR_gp0) {
      return (ue_context_p->ue_context.measGapConfig->
                                            choice.setup.gapOffset.choice.gp0);
    } else if (ue_context_p->ue_context.measGapConfig->
                  choice.setup.gapOffset.present ==
                                      MeasGapConfig__setup__gapOffset_PR_gp1) {
      return (ue_context_p->ue_context.measGapConfig->
                                            choice.setup.gapOffset.choice.gp1);
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
int
mac_eNB_get_rrc_measGap_rep_period(
  const module_id_t module_idP,
  const rnti_t      rntiP
)
//------------------------------------------------------------------------------
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   &eNB_rrc_inst[module_idP],
                   rntiP);

  if (ue_context_p != NULL && ue_context_p->ue_context.measGapConfig != NULL &&
        ue_context_p->ue_context.measGapConfig->present ==
                                                      MeasGapConfig_PR_setup) {
    if (ue_context_p->ue_context.measGapConfig->
            choice.setup.gapOffset.present ==
                                      MeasGapConfig__setup__gapOffset_PR_gp0) {
      /* 3GPP TS 36.133 version 10.20.0 Release 10, section 8.1.2.1 */
      return 40;
    } else if (ue_context_p->ue_context.measGapConfig->
                choice.setup.gapOffset.present ==
                                      MeasGapConfig__setup__gapOffset_PR_gp1) {
      /* 3GPP TS 36.133 version 10.20.0 Release 10, section 8.1.2.1 */
      return 80;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
int is_measGap(
  const module_id_t         module_idP,
  const rnti_t              rnti,
  const lte_frame_type_t    frame_type,
  const frame_t             frameP,
  const sub_frame_t         subframeP
)
//------------------------------------------------------------------------------
{
  // MeasGap implementation
  int gapOffset = mac_eNB_get_rrc_measGap_offset(module_idP, rnti);
  int mgrp = mac_eNB_get_rrc_measGap_rep_period(module_idP, rnti);

  if (gapOffset != -1 && mgrp != -1 &&
                mac_eNB_get_rrc_status(module_idP, rnti) == RRC_RECONFIGURED) {

    int T = (int) mgrp / 10;
    // MeasurementGap Length (MGL, ms) = 6
    int gapLength = 6;
    int total_subfr = 10;
    int total_sfn = 1024;
    int sfnMODt = (int) (frameP % T);
    int floor_gapOffsetDIV10 = (int) floor(gapOffset / 10);
    int gapOffsetMOD10 = (int) (gapOffset % 10);
    int PUSCH_timing = 0;
    if (frame_type == FDD) {
      /* PUSCH transmitted 4ms after DCI transmission. */
      PUSCH_timing = 4;
    } else {
      /* Maximum value among all TDD UL/DL configurations. */
      /* Need to fix it as per UL/DL config. */
      // PUSCH_timing = 7;
    }

    if (gapOffsetMOD10 < PUSCH_timing) {
      int nxt_sfnMODt = (int) ((frameP + 1) % total_sfn) % T;
      /* E.g. if gapOffset = 3, gp1, in SFN 23, don't schedule UE on
       * 9th subframe and in SFN 24 don't schedule UE on subframes
       * 1,2,3,4,5,6,7,8.
       */
      if ((sfnMODt == floor_gapOffsetDIV10) &&
                              (subframeP < (gapOffsetMOD10 + gapLength))) {
        return 1;
      }
      else if ((nxt_sfnMODt == floor_gapOffsetDIV10) &&
              (subframeP >= ((gapOffsetMOD10 - PUSCH_timing) % total_subfr))) {
        return 1;
      }
    }
    else if (gapOffsetMOD10 >= PUSCH_timing) {
      int pre_sfnModt = (int) ((frameP - 1) % total_sfn) % T;
      /* E.g. if gapOffset = 5, gp1, in SFN 24, don't schedule UE on
       * subframes 1,2,3,4,5,6,7,8,9 and in SFN 25 don't schedule UE on
       * 0th subframe.
       */
      if ((sfnMODt == floor_gapOffsetDIV10) &&
                              (subframeP >= (gapOffsetMOD10 - PUSCH_timing))) {
        return 1;
      }
      else if ((pre_sfnModt == floor_gapOffsetDIV10) &&
              (subframeP < ((gapOffsetMOD10 + gapLength) % total_subfr))) {
        return 1;
      }
    }
  }
  return 0;
}

void mac_eNB_rrc_ul_failure(const module_id_t Mod_instP,
			    const int CC_idP,
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rntiP)
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   &eNB_rrc_inst[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_I(RRC,"Frame %d, Subframe %d: UE %x UL failure, activating timer\n",frameP,subframeP,rntiP);
    ue_context_p->ue_context.ul_failure_timer=1;
  }
  else {
    LOG_W(RRC,"Frame %d, Subframe %d: UL failure: UE %x unknown \n",frameP,subframeP,rntiP);
  }
  rrc_mac_remove_ue(Mod_instP,rntiP);
}

void mac_eNB_rrc_ul_in_sync(const module_id_t Mod_instP,
			    const int CC_idP,
			    const frame_t frameP,
			    const sub_frame_t subframeP,
			    const rnti_t rntiP)
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  ue_context_p = rrc_eNB_get_ue_context(
                   &eNB_rrc_inst[Mod_instP],
                   rntiP);

  if (ue_context_p != NULL) {
    LOG_I(RRC,"Frame %d, Subframe %d: UE %x to UL in synch\n",
          frameP, subframeP, rntiP);
    ue_context_p->ue_context.ul_failure_timer = 0;
  } else {
    LOG_E(RRC,"Frame %d, Subframe %d: UE %x unknown \n",
          frameP, subframeP, rntiP);
  }
}
//------------------------------------------------------------------------------
int
mac_UE_get_rrc_status(
  const module_id_t Mod_idP,
  const uint8_t     indexP
)
//------------------------------------------------------------------------------
{
  return(UE_rrc_inst[Mod_idP].Info[indexP].State);
}

//-------------------------------------------------------------------------------------------//
int mac_ue_ccch_success_ind(module_id_t Mod_idP, uint8_t eNB_index)
{
  //-------------------------------------------------------------------------------------------//
#if defined(ENABLE_ITTI)
  {
    MessageDef *message_p;

    message_p = itti_alloc_new_message (TASK_MAC_UE, RRC_MAC_CCCH_DATA_CNF);
    RRC_MAC_CCCH_DATA_CNF (message_p).enb_index = eNB_index;

    itti_send_msg_to_task (TASK_RRC_UE, UE_MODULE_ID_TO_INSTANCE(Mod_idP), message_p);
  }
#else
  // reset the tx buffer to indicate RRC that ccch was successfully transmitted (for example if contention resolution succeeds)
  UE_rrc_inst[Mod_idP].Srb0[eNB_index].Tx_buffer.payload_size=0;
#endif
  return 0;
}
