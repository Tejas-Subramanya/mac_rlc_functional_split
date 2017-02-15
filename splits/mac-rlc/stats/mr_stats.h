/* Copyright (c) 2017 Kewin Rausch
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

/* Data indicator statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_ind_prologue(void);
void mr_stat_ind_epilogue(void);

/* Data request statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_req_prologue(void);
void mr_stat_req_epilogue(void);

/* Status indicator statistic collection procedures.
 * Prologue: to be invoked just before starting the operations.
 * Epilogue: to be invoked just after ending the operations.
 */
void mr_stat_status_prologue(void);
void mr_stat_status_epilogue(void);

#endif /* __MR_STATS_H */
