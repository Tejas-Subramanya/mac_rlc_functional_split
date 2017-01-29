// License to be added FBK CREATE-NET

#include <pthread.h>
#include <stdint.h>

#ifndef SPLIT_MACRLC_CU_TASK_H_
#define SPLIT_MACRLC_CU_TASK_H_
#include "enb_config.h"
#include "intertask_interface_types.h"

/** \brief UDP RLC CU task on eNB.
 *  \param args_p
 *  @returns always NULL
 */
void *udp_rlc_cu_task(void *args_p);

/** \brief init UDP RLC CU task.
 *  \param enb_config_p configuration of eNB
 *  @returns always 0
 */
int udp_rlc_cu_init(const Enb_properties_t *enb_config_p);

#endif 
