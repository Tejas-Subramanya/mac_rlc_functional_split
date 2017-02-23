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
# include <signal.h>

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
#     include "mr_cu.h"
#     include "rlc.h"
#   endif

#   if defined(SPLIT_MAC_RLC_DU)
#     include <pthread.h>
#     include <unistd.h>
#     include "mr_du.h"
#     include "rlc.h"
#   endif
# endif
# include "enb_app.h"

#ifdef SPLIT_MAC_RLC_DU
void * dummy_loop(void * args) {
  char data[1024];

  while(1) {
    mr_stat_ind_prologue();
    du_send(data, 1024);

    usleep(1000);
  }
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

#ifdef SPLIT_MAC_RLC_CU
    if (enb_nb > 0) {
      // Arguments for UDP netw are "destination_ip:local_listening_port".
      if (cu_init("192.168.100.100:9001", mac_rlc_cu_recv)) {
        LOG_E(SPLIT_MAC_RLC_CU, "Create thread for  SPLIT MACRLC CU failed\n");
        return -1;
      }
    }
#endif

#ifdef SPLIT_MAC_RLC_DU
    pthread_t dummy_t;

    if (enb_nb > 0) {
      // Arguments for UDP netw are "destination_ip:local_listening_port".
      if (du_init("192.168.100.101:9001", mac_rlc_du_recv)) {
        LOG_E(SPLIT_MAC_RLC_DU, "Create thread for SPLIT MACRLC DU failed\n");
        return -1;
      }

      //pthread_create(&dummy_t, 0, dummy_loop, 0);
    }
#endif
  }
# endif // openair2: NN: should be openair3


  itti_wait_ready(0);

  return 0;
}
#endif
