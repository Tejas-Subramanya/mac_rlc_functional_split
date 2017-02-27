/* Copyright (c) 2017 Kewin Rausch and Tejas Subramanya
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

#ifndef __MR_STATS_H
#define __MR_STATS_H

/* MAC-RLC Data indicator statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_ind_prologue(uint32_t req_size);
void mr_stat_ind_epilogue(void);

/* MAC-RLC Data request statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_req_prologue(uint32_t req_size);
void mr_stat_req_epilogue(uint32_t res_size);

/* Status indicator statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_status_prologue(uint32_t req_size);
void mr_stat_status_epilogue(uint32_t res_size);

/* MAC-RRC Data request statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */

void mr_stat_rrc_req_prologue(uint32_t req_size);
void mr_stat_rrc_req_epilogue(uint32_t res_size);

/* MAC-RRC Data indicator statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_rrc_ind_prologue(uint32_t req_size);
void mr_stat_rrc_ind_epilogue(void);

#endif /* __MR_STATS_H */
