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

#define bytes_to_Mb(x)	(((float)x * 8.0) / 1000000.0)

/* One second in ns. */
#define REP_TIEMOUT	1000000000

/* Ok, these are shared elements used to evaluate how many nsec passed from the
 * last time we did a reported something.
 */
uint64_t        rep_elapsed_nsec = 0;
struct timespec rep_now          = {0};
struct timespec rep_last         = {0};

/* Prototype for the reporting procedure. */
static inline void mr_stat_report(void);

/*
 * MAC-RLC Data Indicator stuff:
 */

struct timespec data_ind_start  = {0};
struct timespec data_ind_finish = {0};

/* Stats */
uint64_t        data_ind_count_req = 0;
uint64_t        data_ind_total = 0;
uint64_t        data_ind_sent_bytes = 0;

void mr_stat_ind_prologue(uint32_t req_size) {
	clock_gettime(CLOCK_REALTIME, &data_ind_start);
        data_ind_sent_bytes += req_size;
        data_ind_count_req++;
}

void mr_stat_ind_epilogue() {
	clock_gettime(CLOCK_REALTIME, &rep_now);

	/* Report if it's the right time... */
	mr_stat_report();
}

/*
 * MAC-RLC Data request stuff:
 */

struct timespec data_req_start  = {0};
struct timespec data_req_finish = {0};

/* Statistics: */
uint64_t        data_req_count_res = 0;
uint64_t        data_req_count_req = 0;
uint64_t        data_req_total = 0;
uint64_t        data_req_recv_bytes = 0;
uint64_t        data_req_sent_bytes = 0;

void mr_stat_req_prologue(uint32_t req_size) {
	clock_gettime(CLOCK_REALTIME, &data_req_start);
	data_req_sent_bytes += req_size;
	data_req_count_req++;
}

void mr_stat_req_epilogue(uint32_t res_size) {
	long int e;

	clock_gettime(CLOCK_REALTIME, &data_req_finish);
	e = ts_nsec(data_req_finish) - ts_nsec(data_req_start);

	data_req_count_res++;
	data_req_total += e;
	data_req_recv_bytes += res_size;

	/* Report if it's the right time... */
	mr_stat_report();
}

/*
 * Status indicator stuff:
 */

struct timespec status_start  = {0};
struct timespec status_finish = {0};

/* Statistics: */
uint64_t        status_count_res = 0;
uint64_t        status_count_req = 0;
uint64_t        status_total = 0;
uint64_t        status_recv_bytes = 0;
uint64_t        status_sent_bytes = 0;

void mr_stat_status_prologue(uint32_t req_size) {
	clock_gettime(CLOCK_REALTIME, &status_start);
	status_sent_bytes += req_size;
	status_count_req++;
}

void mr_stat_status_epilogue(uint32_t res_size) {

	long int e;

	clock_gettime(CLOCK_REALTIME, &status_finish);
	e = ts_nsec(status_finish) - ts_nsec(status_start);

	status_count_res++;
	status_total += e;
	status_recv_bytes += res_size;

	/* Report if it's the right time... */
	mr_stat_report();
}

/*
 * MAC-RRC Data request stuff:
 */

struct timespec mr_stat_rrc_req_start  = {0};
struct timespec mr_stat_rrc_req_finish = {0};

/* Statistics: */
uint64_t        mr_stat_rrc_req_count_res = 0;
uint64_t        mr_stat_rrc_req_count_req = 0;
uint64_t        mr_stat_rrc_req_total = 0;
uint64_t        mr_stat_rrc_req_recv_bytes = 0;
uint64_t        mr_stat_rrc_req_sent_bytes = 0;

void mr_stat_rrc_req_prologue(uint32_t req_size) {
	clock_gettime(CLOCK_REALTIME, &mr_stat_rrc_req_start);
	mr_stat_rrc_req_sent_bytes += req_size;
	mr_stat_rrc_req_count_req++;
}

void mr_stat_rrc_req_epilogue(uint32_t res_size) {
	long int e;

	clock_gettime(CLOCK_REALTIME, &mr_stat_rrc_req_finish);
	e = ts_nsec(mr_stat_rrc_req_finish) - ts_nsec(mr_stat_rrc_req_start);

	mr_stat_rrc_req_count_res++;
	mr_stat_rrc_req_total += e;
	mr_stat_rrc_req_recv_bytes += res_size;

	/* Report if it's the right time... */
	mr_stat_report();
}

/*
 * MAC-RRC Data Indicator stuff:
 */

struct timespec mr_stat_rrc_ind_start  = {0};
struct timespec mr_stat_rrc_ind_finish = {0};

/* Stats */
uint64_t        mr_stat_rrc_ind_count_req = 0;
uint64_t        mr_stat_rrc_ind_total = 0;
uint64_t        mr_stat_rrc_ind_sent_bytes = 0;

void mr_stat_rrc_ind_prologue(uint32_t req_size) {
	clock_gettime(CLOCK_REALTIME, &mr_stat_rrc_ind_start);
	mr_stat_rrc_ind_sent_bytes += req_size;
	mr_stat_rrc_ind_count_req++;
}

void mr_stat_rrc_ind_epilogue() {
	/* Report if it's the right time... */
	mr_stat_report();
}

/* The actual reporting procedure.
 *
 * MULTITHREADING NOTE: This procedure will be called by mainly by DU receiver
 * worker thread, and so the threading context is already considered safe. In
 * case where multiple workers are introduced, the data must be protected with
 * a lock that can be defined privately in this code sheet.
 */
static inline void mr_stat_report(void) {
	if(ts_nsec(rep_now) - ts_nsec(rep_last) > REP_TIEMOUT) {
		/*
		 * Dump all the statistics at once:
		 */

		printf(
"***************************************************************************\n"
"MAC-RLC split collected statistics:\n"
"   Elapsed time: %.3f ms\n"
"   DATA IND   Requests=%-10llu Responses=%-10llu Avg.RTT(usec)=%-10.3f "
"sent=%-10llu recv=%-10llu\n"
"   DATA REQ   Requests=%-10llu Responses=%-10llu Avg.RTT(usec)=%-10.3f "
"sent=%-10llu recv=%-10llu\n"
"   STATUS IND Requests=%-10llu Responses=%-10llu Avg.RTT(usec)=%-10.3f "
"sent=%-10llu recv=%-10llu\n"
"   RRC IND    Requests=%-10llu Responses=%-10llu Avg.RTT(usec)=%-10.3f "
"sent=%-10llu recv=%-10llu\n"
"   RRC REQ    Requests=%-10llu Responses=%-10llu Avg.RTT(usec)=%-10.3f "
"sent=%-10llu recv=%-10llu\n"
"***************************************************************************\n",
			/*
			 * Time elapsed from last report:
			 */
			(float)(ts_nsec(rep_now) - ts_nsec(rep_last)) /
				1000000.0,
			/*
			 * Data indicator:
			 */
			data_ind_count_req,
			0ULL,
			0.0,
			data_ind_sent_bytes,
			0ULL,
			/*
			 * Data requests:
			 */
			data_req_count_req,
			data_req_count_res,
			(float)data_req_total / (float)data_req_count_res /
				1000.0,
			data_req_sent_bytes,
			data_req_recv_bytes,
			/*
			 * Status indicator:
			 */
			status_count_req,
			status_count_res,
			(float)status_total / (float)status_count_res / 1000.0,
			status_sent_bytes,
			status_recv_bytes,
			/*
			 * RRC request:
			 */
			mr_stat_rrc_ind_count_req,
			0ULL,
			0.0,
			mr_stat_rrc_ind_sent_bytes,
			0ULL,
			/*
			 * RRC indicator:
			 */
			mr_stat_rrc_req_count_req,
			mr_stat_rrc_req_count_res,
			(float)mr_stat_rrc_req_total /
				(float)mr_stat_rrc_req_count_res / 1000.0,
			mr_stat_rrc_req_sent_bytes,
			mr_stat_rrc_req_recv_bytes);

		/*
		 * Variables reset for the next run:
		 */

		data_ind_count_req          = 0;
		data_ind_sent_bytes         = 0;

		data_req_total              = 0;
		data_req_count_res          = 0;
		data_req_count_req          = 0;
		data_req_recv_bytes         = 0;
		data_req_sent_bytes         = 0;

		status_total                = 0;
		status_count_res            = 0;
		status_count_req            = 0;
		status_recv_bytes           = 0;
		status_sent_bytes           = 0;

		mr_stat_rrc_req_total       = 0;
		mr_stat_rrc_req_count_res   = 0;
		mr_stat_rrc_req_count_req   = 0;
		mr_stat_rrc_req_recv_bytes  = 0;
		mr_stat_rrc_req_sent_bytes  = 0;

		mr_stat_rrc_ind_count_req   = 0;
		mr_stat_rrc_ind_sent_bytes  = 0;

		/*
		 * Remember when you last performed report:
		 */

		rep_last.tv_sec  = rep_now.tv_sec;
		rep_last.tv_nsec = rep_now.tv_nsec;
	}
}
