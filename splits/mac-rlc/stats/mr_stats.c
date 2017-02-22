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

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <mr_stats.h>

/* Convert TS structure to nanoseconds: */
#define ts_nsec(t)	((t.tv_sec * 1000000000) + (t.tv_nsec))
/* Convert TS structure to microseconds: */
#define ts_usec(t)	((t.tv_sec * 1000000) + (t.tv_nsec / 1000))
/* Convert TS structure to milliseconds: */
#define ts_msec(t)	((t.tv_sec * 1000) + (t.tv_nsec / 1000000))

/*
 * Data indicator stuff:
 */

#define MR_STAT_IND_SHOW

struct timespec ind_start  = {0};
struct timespec ind_finish = {0};

uint64_t        ind_rtt_count = 0;
uint64_t        ind_rtt_total = 0;

void mr_stat_ind_prologue() {
	clock_gettime(CLOCK_REALTIME, &ind_start);
}

void mr_stat_ind_epilogue() {
	long int e;

	clock_gettime(CLOCK_REALTIME, &ind_finish);

	e = ts_nsec(ind_finish) - ts_nsec(ind_start);

	ind_rtt_count++;
	ind_rtt_total += e;

#ifdef MR_STAT_IND_SHOW
	if(ind_rtt_count == 1000) {
		printf("MAC-RLC status indicator statistics:\n"
			"    Cycles:           %d\n"
			"    Avg running time: %.3f usec\n",
			ind_rtt_total,
			((float)ind_rtt_total / (float)ind_rtt_count) / 1000.0);

		ind_rtt_count = 0;
		ind_rtt_total = 0;
	}
#endif
}

/*
 * Data request stuff:
 */

struct timespec req_start  = {0};
struct timespec req_finish = {0};

void mr_stat_req_prologue() {
	clock_gettime(CLOCK_REALTIME, &req_start);
}

void mr_stat_req_epilogue() {
	clock_gettime(CLOCK_REALTIME, &req_finish);
}

/*
 * Status indicator stuff:
 */

struct timespec status_start  = {0};
struct timespec status_finish = {0};

void mr_stat_status_prologue() {
	clock_gettime(CLOCK_REALTIME, &status_start);
}

void mr_stat_status_epilogue() {
	clock_gettime(CLOCK_REALTIME, &status_finish);
        printf("Print stats for 'status_req&rep' messages\n");
}
