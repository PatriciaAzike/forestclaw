/*
Copyright (c) 2012 Carsten Burstedde, Donna Calhoun
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fclaw_timer.h>


#include <fclaw2d_forestclaw.h>
#include <fclaw2d_partition.h>
#include <sc_statistics.h>
#include <fclaw2d_domain.h>

#define FCLAW2D_STATS_SET(stats,ddata,NAME) do {                               \
    SC_CHECK_ABORT (!(ddata)->timers[FCLAW2D_TIMER_ ## NAME].running,          \
                    "Timer " #NAME " still running in fclaw2d_domain_finalize");              \
    sc_stats_set1 ((stats) + FCLAW2D_TIMER_ ## NAME,                           \
                   (ddata)->timers[FCLAW2D_TIMER_ ## NAME].cumulative, #NAME); \
} while (0)

/* -----------------------------------------------------------------
   Work with timers
   ----------------------------------------------------------------- */

double
fclaw2d_timer_wtime (void)
{
    return sc_MPI_Wtime ();
}

void
fclaw2d_timer_init (fclaw2d_timer_t *timer)
{
    memset (timer, 0, sizeof (fclaw2d_timer_t));
}

void
fclaw2d_timer_start (fclaw2d_timer_t *timer)
{
    if (!timer->running) {
        timer->started = fclaw2d_timer_wtime ();
        timer->stopped = 0.;
        timer->running = 1;
    }
    else
    {
        SC_ABORT_NOT_REACHED ();
    }
}

void
fclaw2d_timer_stop (fclaw2d_timer_t *timer)
{
    if (timer->running) {
        timer->stopped = fclaw2d_timer_wtime ();
        timer->cumulative += timer->stopped - timer->started;
        timer->running = 0;
    }
    else
    {
        SC_ABORT_NOT_REACHED ();
    }
}

void
fclaw2d_timer_report(fclaw2d_domain_t *domain)
{
    fclaw2d_domain_data_t *ddata = fclaw2d_domain_get_data (domain);

    sc_statinfo_t stats[FCLAW2D_TIMER_COUNT];

    fclaw2d_timer_stop (&ddata->timers[FCLAW2D_TIMER_WALLTIME]);

    FCLAW2D_STATS_SET (stats, ddata, INIT);
    FCLAW2D_STATS_SET (stats, ddata, OUTPUT);
    FCLAW2D_STATS_SET (stats, ddata, DIAGNOSTICS);
    FCLAW2D_STATS_SET (stats, ddata, REGRID);
    FCLAW2D_STATS_SET (stats, ddata, ADVANCE);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL);
    FCLAW2D_STATS_SET (stats, ddata, ADAPT_COMM);
    FCLAW2D_STATS_SET (stats, ddata, PARTITION_COMM);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTPATCH_COMM);
    FCLAW2D_STATS_SET (stats, ddata, DIAGNOSTICS_COMM);
    FCLAW2D_STATS_SET (stats, ddata, CFL_COMM);
    FCLAW2D_STATS_SET (stats, ddata, WALLTIME);
    FCLAW2D_STATS_SET (stats, ddata, ADVANCE_STEPS_COUNTER);
    FCLAW2D_STATS_SET (stats, ddata, REGRID_BUILD);
    FCLAW2D_STATS_SET (stats, ddata, REGRID_TAGGING);
    FCLAW2D_STATS_SET (stats, ddata, PARTITION);
    FCLAW2D_STATS_SET (stats, ddata, PARTITION_BUILD);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTPATCH_BUILD);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_COPY);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_AVERAGE);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_INTERP);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_PHYSBC);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_STEP1);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_STEP2);
    FCLAW2D_STATS_SET (stats, ddata, GHOSTFILL_STEP3);
    FCLAW2D_STATS_SET (stats, ddata, NEIGHBOR_SEARCH);
    FCLAW2D_STATS_SET (stats, ddata, EXTRA1);
    FCLAW2D_STATS_SET (stats, ddata, EXTRA2);
    FCLAW2D_STATS_SET (stats, ddata, EXTRA3);
    FCLAW2D_STATS_SET (stats, ddata, EXTRA4);

    /* Compute the inverse harmonic mean.  */
    ddata->count_single_step += 1;   /* Avoid division by zero */
    sc_stats_set1 (&stats[FCLAW2D_TIMER_ADVANCE_STEPS_HMEAN],
                   1.0/ddata->count_single_step,"ADVANCE_STEPS_HMEAN");

    sc_stats_set1 (&stats[FCLAW2D_TIMER_UNACCOUNTED],
                   ddata->timers[FCLAW2D_TIMER_WALLTIME].cumulative -
                   (ddata->timers[FCLAW2D_TIMER_INIT].cumulative +
                    ddata->timers[FCLAW2D_TIMER_REGRID].cumulative +
                    ddata->timers[FCLAW2D_TIMER_OUTPUT].cumulative +
                    ddata->timers[FCLAW2D_TIMER_DIAGNOSTICS].cumulative +
                    ddata->timers[FCLAW2D_TIMER_ADVANCE].cumulative +
                    ddata->timers[FCLAW2D_TIMER_GHOSTFILL].cumulative +
                    ddata->timers[FCLAW2D_TIMER_ADAPT_COMM].cumulative +
                    ddata->timers[FCLAW2D_TIMER_GHOSTPATCH_COMM].cumulative +
                    ddata->timers[FCLAW2D_TIMER_PARTITION_COMM].cumulative +
                    ddata->timers[FCLAW2D_TIMER_DIAGNOSTICS_COMM].cumulative +
                    ddata->timers[FCLAW2D_TIMER_CFL_COMM].cumulative),
                   "UNACCOUNTED");

    sc_stats_set1 (&stats[FCLAW2D_TIMER_COMM],
                   ddata->timers[FCLAW2D_TIMER_ADAPT_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_GHOSTPATCH_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_PARTITION_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_DIAGNOSTICS_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_CFL_COMM].cumulative,
                   "FCLAW2D_TIMER_COMM");

    /* Just subtracting FCLAW2D_TIMER_COMM here doesn't work ... */
    sc_stats_set1 (&stats[FCLAW2D_TIMER_LOCAL],
                   ddata->timers[FCLAW2D_TIMER_WALLTIME].cumulative -
                   (ddata->timers[FCLAW2D_TIMER_ADAPT_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_GHOSTPATCH_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_PARTITION_COMM].cumulative +
                   ddata->timers[FCLAW2D_TIMER_DIAGNOSTICS_COMM].cumulative +
                    ddata->timers[FCLAW2D_TIMER_CFL_COMM].cumulative),
                   "FCLAW2D_TIMER_LOCAL");


    sc_stats_compute (domain->mpicomm, FCLAW2D_TIMER_COUNT, stats);

    sc_stats_print (sc_package_id, SC_LP_ESSENTIAL, FCLAW2D_TIMER_COUNT,
                    stats, 1, 0);

    SC_GLOBAL_ESSENTIALF ("Procs %d advance %d %g exchange %d %g "
                          "regrid %d %g\n", domain->mpisize,
                          ddata->count_amr_advance,
                          stats[FCLAW2D_TIMER_ADVANCE].average,
                          ddata->count_ghost_exchange,
                          stats[FCLAW2D_TIMER_GHOSTFILL].average,
                          ddata->count_amr_regrid,
                          stats[FCLAW2D_TIMER_REGRID].average);

    SC_GLOBAL_ESSENTIALF ("Max/P %d advance %d %g exchange %d %g "
                          "regrid %d %g\n", domain->mpisize,
                          ddata->count_amr_advance,
                          stats[FCLAW2D_TIMER_ADVANCE].max,
                          ddata->count_ghost_exchange,
                          stats[FCLAW2D_TIMER_GHOSTFILL].max,
                          ddata->count_amr_regrid,
                          stats[FCLAW2D_TIMER_REGRID].max);

#if 0
    /* Find out process rank */
    /* TODO : Fix this so that it doesn't interfere with output printed above. */
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Write out individual processor timers */
    printf("%12s time on proc %d : %12.4f\n","ADVANCE",
           domain->mpirank,ddata->timers[FCLAW2D_TIMER_ADVANCE].cumulative);
    printf("%12s time on proc %d : %12.4f\n","GHOSTCOMM",
           domain->mpirank,ddata->timers[FCLAW2D_TIMER_GHOSTCOMM].cumulative);
    printf("%12s time on proc %d : %12.4f\n","EXCHANGE",
           domain->mpirank,ddata->timers[FCLAW2D_TIMER_EXCHANGE].cumulative);
    printf("%12s time on proc %d : %12.4f\n","REGRID",
           domain->mpirank,ddata->timers[FCLAW2D_TIMER_REGRID].cumulative);
    printf("\n");
#endif


}
