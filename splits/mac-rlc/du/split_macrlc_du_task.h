// License to be added FBK CREATE-NET

#include <pthread.h>
#include <stdint.h>

#ifndef SPLIT_MACRLC_DU_TASK_H_
#define SPLIT_MACRLC_DU_TASK_H_
#include "enb_config.h"
#include "intertask_interface_types.h"

/** \brief UDP MAC DU task on eNB.
 *  \param args_p
 *  @returns always NULL
 */
void *udp_mac_du_task(void *args_p);

/** \brief init UDP MAC DU task.
 *  \param enb_config_p configuration of eNB
 *  @returns always 0
 */
int udp_mac_du_init(const Enb_properties_t *enb_config_p);

#endif 
