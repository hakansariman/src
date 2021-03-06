/*	$OpenBSD: rf_acctrace.c,v 1.4 2002/12/16 07:01:02 tdeval Exp $	*/
/*	$NetBSD: rf_acctrace.c,v 1.4 1999/08/13 03:41:52 oster Exp $	*/

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*****************************************************************************
 *
 * acctrace.c -- Code to support collecting information about each access.
 *
 *****************************************************************************/


#include <sys/stat.h>
#include <sys/types.h>

#include "rf_threadstuff.h"
#include "rf_types.h"
#include "rf_debugMem.h"
#include "rf_acctrace.h"
#include "rf_general.h"
#include "rf_raid.h"
#include "rf_etimer.h"
#include "rf_hist.h"
#include "rf_shutdown.h"

static long numTracesSoFar;
static int accessTraceBufCount = 0;
static RF_AccTraceEntry_t *access_tracebuf;
static long traceCount;

int	rf_stopCollectingTraces;
RF_DECLARE_MUTEX(rf_tracing_mutex);
int	rf_trace_fd;

void rf_ShutdownAccessTrace(void *);

void
rf_ShutdownAccessTrace(void *ignored)
{
	if (rf_accessTraceBufSize) {
		if (accessTraceBufCount)
			rf_FlushAccessTraceBuf();
		RF_Free(access_tracebuf, rf_accessTraceBufSize *
		    sizeof(RF_AccTraceEntry_t));
	}
	rf_mutex_destroy(&rf_tracing_mutex);
}

int
rf_ConfigureAccessTrace(RF_ShutdownList_t **listp)
{
	int rc;

	numTracesSoFar = accessTraceBufCount = rf_stopCollectingTraces = 0;
	if (rf_accessTraceBufSize) {
		RF_Malloc(access_tracebuf, rf_accessTraceBufSize *
		    sizeof(RF_AccTraceEntry_t), (RF_AccTraceEntry_t *));
		accessTraceBufCount = 0;
	}
	traceCount = 0;
	numTracesSoFar = 0;
	rc = rf_mutex_init(&rf_tracing_mutex);
	if (rc) {
		RF_ERRORMSG3("Unable to init mutex file %s line %d rc=%d.\n",
		    __FILE__, __LINE__, rc);
	}
	rc = rf_ShutdownCreate(listp, rf_ShutdownAccessTrace, NULL);
	if (rc) {
		RF_ERRORMSG3("Unable to add to shutdown list file %s line %d"
		    " rc=%d.\n", __FILE__, __LINE__, rc);
		if (rf_accessTraceBufSize) {
			RF_Free(access_tracebuf, rf_accessTraceBufSize *
			    sizeof(RF_AccTraceEntry_t));
			rf_mutex_destroy(&rf_tracing_mutex);
		}
	}
	return (rc);
}

/*
 * Install a trace record. Cause a flush to disk or to the trace collector
 * daemon if the trace buffer is at least 1/2 full.
 */
void
rf_LogTraceRec(RF_Raid_t *raid, RF_AccTraceEntry_t *rec)
{
	RF_AccTotals_t *acc = &raid->acc_totals;
#if 0
	RF_Etimer_t timer;
	int	i, n;
#endif

	if (rf_stopCollectingTraces || ((rf_maxNumTraces >= 0) &&
	    (numTracesSoFar >= rf_maxNumTraces)))
		return;

	/* Update AccTotals for this device. */
	if (!raid->keep_acc_totals)
		return;

	acc->num_log_ents++;
	if (rec->reconacc) {
		acc->recon_start_to_fetch_us +=
		    rec->specific.recon.recon_start_to_fetch_us;
		acc->recon_fetch_to_return_us +=
		    rec->specific.recon.recon_fetch_to_return_us;
		acc->recon_return_to_submit_us +=
		    rec->specific.recon.recon_return_to_submit_us;
		acc->recon_num_phys_ios += rec->num_phys_ios;
		acc->recon_phys_io_us += rec->phys_io_us;
		acc->recon_diskwait_us += rec->diskwait_us;
		acc->recon_reccount++;
	} else {
		RF_HIST_ADD(acc->tot_hist, rec->total_us);
		RF_HIST_ADD(acc->dw_hist, rec->diskwait_us);
		/*
		 * Count of physical IOs that are too big. (often due to
		 * thermal recalibration)
		 *
		 * If bigvals > 0, you should probably ignore this data set.
		 */
		if (rec->diskwait_us > 100000)
			acc->bigvals++;

		acc->total_us += rec->total_us;
		acc->suspend_ovhd_us += rec->specific.user.suspend_ovhd_us;
		acc->map_us += rec->specific.user.map_us;
		acc->lock_us += rec->specific.user.lock_us;
		acc->dag_create_us += rec->specific.user.dag_create_us;
		acc->dag_retry_us += rec->specific.user.dag_retry_us;
		acc->exec_us += rec->specific.user.exec_us;
		acc->cleanup_us += rec->specific.user.cleanup_us;
		acc->exec_engine_us += rec->specific.user.exec_engine_us;
		acc->xor_us += rec->xor_us;
		acc->q_us += rec->q_us;
		acc->plog_us += rec->plog_us;
		acc->diskqueue_us += rec->diskqueue_us;
		acc->diskwait_us += rec->diskwait_us;
		acc->num_phys_ios += rec->num_phys_ios;
		acc->phys_io_us = rec->phys_io_us;
		acc->user_reccount++;
	}
}


/*
 * Assumes the tracing mutex is locked at entry. In order to allow this to
 * be called from interrupt context, we don't do any copyouts here, but rather
 * just wake the trace buffer collector thread.
 */
void
rf_FlushAccessTraceBuf(void)
{
	accessTraceBufCount = 0;
}
