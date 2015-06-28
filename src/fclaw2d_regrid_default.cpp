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

#include <fclaw2d_forestclaw.h>
#include <fclaw2d_vtable.h>
#include <fclaw2d_clawpatch.h>

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif


int fclaw2d_patch_tag4refinement(fclaw2d_domain_t *domain,
                                 fclaw2d_patch_t *this_patch,
                                 int blockno, int patchno,
                                 int initflag)
{
    fclaw2d_vtable_t vt;
    int mx,my,mbc,meqn;
    double xlower,ylower,dx,dy;
    double *q;
    int tag_patch;
    double refine_threshold;
    const amr_options_t *amropt;

    amropt = get_domain_parms(domain);
    refine_threshold = amropt->refine_threshold;

    vt = fclaw2d_get_vtable(domain);

    fclaw2d_clawpatch_grid_data(domain,this_patch,&mx,&my,&mbc,
                                &xlower,&ylower,&dx,&dy);

    fclaw2d_clawpatch_soln_data(domain,this_patch,&q,&meqn);

    tag_patch = 0;
    vt.fort_tag4refinement(&mx,&my,&mbc,&meqn,&xlower,&ylower,&dx,&dy,
                           &blockno, q,&refine_threshold,
                           &initflag,&tag_patch);
    return tag_patch;
}

int fclaw2d_patch_tag4coarsening(fclaw2d_domain_t *domain,
                                 fclaw2d_patch_t *fine_patches,
                                 int blockno,
                                 int patchno)
{
    fclaw2d_vtable_t vt;

    int mx,my,mbc,meqn;
    double xlower,ylower,dx,dy;
    double *q[4];
    int tag_patch,igrid;
    double coarsen_threshold;
    fclaw2d_patch_t *patch0;

    patch0 = &fine_patches[0];

    const amr_options_t *amropt;
    amropt = get_domain_parms(domain);

    coarsen_threshold = amropt->coarsen_threshold;

    vt = fclaw2d_get_vtable(domain);

    fclaw2d_clawpatch_grid_data(domain,patch0,&mx,&my,&mbc,
                                &xlower,&ylower,&dx,&dy);

    for (igrid = 0; igrid < 4; igrid++)
    {
        fclaw2d_clawpatch_soln_data(domain,&fine_patches[igrid],&q[igrid],&meqn);
    }

    tag_patch = 0;
    vt.fort_tag4coarsening(&mx,&my,&mbc,&meqn,&xlower,&ylower,&dx,&dy,
                           &blockno, q[0],q[1],q[2],q[3],
                           &coarsen_threshold,&tag_patch);
    return tag_patch == 1;
}


/* -----------------------------------------------------------------
   Callback routine for tagging
   ----------------------------------------------------------------- */
void fclaw2d_patch_copy2samesize(fclaw2d_domain_t* domain,
                                 fclaw2d_patch_t *old_patch,
                                 fclaw2d_patch_t* new_patch,
                                 int blockno, int old_patchno,
                                 int new_patchno)
{
    int mx,my,mbc,meqn;
    double *qold, *qnew;
    const amr_options_t* gparms;

    gparms = get_domain_parms(domain);
    mx = gparms->mx;
    my = gparms->my;
    mbc = gparms->mbc;

    fclaw2d_clawpatch_soln_data(domain,old_patch,&qold,&meqn);
    fclaw2d_clawpatch_soln_data(domain,new_patch,&qnew,&meqn);

    memcpy(qnew,qold,meqn*(mx+2*mbc)*(my+2*mbc)*sizeof(double));
}


void fclaw2d_patch_interpolate2fine(fclaw2d_domain_t* domain,
                                    fclaw2d_patch_t *coarse_patch,
                                    fclaw2d_patch_t* fine_patches,
                                    int this_blockno, int coarse_patchno,
                                    int fine0_patchno)

{
    fclaw2d_vtable_t vt;
    int mx,my,mbc,meqn;
    double *qcoarse,*qfine;
    double *areacoarse,*areafine;
    double *xp,*yp,*zp,*xd,*yd,*zd;
    int igrid, fine_patchno;

    const amr_options_t* gparms;
    fclaw2d_patch_t* fine_patch;

    vt = fclaw2d_get_vtable(domain);

    gparms = get_domain_parms(domain);
    mx = gparms->mx;
    my = gparms->my;
    mbc = gparms->mbc;

    fclaw2d_clawpatch_metric_data(domain,coarse_patch,&xp,&yp,&zp,
                                  &xd,&yd,&zd,&areacoarse);
    fclaw2d_clawpatch_soln_data(domain,coarse_patch,&qcoarse,&meqn);

    /* Loop over four siblings (z-ordering) */
    for (igrid = 0; igrid < 4; igrid++)
    {
        fine_patch = &fine_patches[igrid];
        fine_patchno = fine0_patchno + igrid;

        fclaw2d_clawpatch_soln_data(domain,fine_patch,&qfine,&meqn);

        if (gparms->manifold)
        {
            fclaw2d_clawpatch_metric_data(domain,fine_patch,&xp,&yp,&zp,
                                          &xd,&yd,&zd,&areafine);
        }

        vt.fort_interpolate2fine(&mx,&my,&mbc,&meqn,qcoarse,qfine,
                                 areacoarse, areafine, &igrid,
                                 &gparms->manifold);

    }
}

void fclaw2d_patch_average2coarse(fclaw2d_domain_t *domain,
                                  fclaw2d_patch_t *fine_patches,
                                  fclaw2d_patch_t *coarse_patch,
                                  int blockno, int fine0_patchno,
                                  int coarse_patchno)

{
    fclaw2d_vtable_t vt;
    const amr_options_t* gparms;
    int mx,my, mbc, meqn;
    double *qcoarse, *qfine;
    double *areacoarse, *areafine;
    double *xp,*yp,*zp,*xd,*yd,*zd;
    int igrid, fine_patchno;
    fclaw2d_patch_t *fine_patch;

    vt = fclaw2d_get_vtable(domain);

    gparms = get_domain_parms(domain);
    mx = gparms->mx;
    my = gparms->my;
    mbc = gparms->mbc;

    fclaw2d_clawpatch_metric_data(domain,coarse_patch,&xp,&yp,&zp,
                                  &xd,&yd,&zd,&areacoarse);
    fclaw2d_clawpatch_soln_data(domain,coarse_patch,&qcoarse,&meqn);

    for(igrid = 0; igrid < 4; igrid++)
    {
        fine_patch = &fine_patches[igrid];
        fine_patchno = fine0_patchno + igrid;

        fclaw2d_clawpatch_soln_data(domain,fine_patch,&qfine,&meqn);

        if (gparms->manifold)
        {
            fclaw2d_clawpatch_metric_data(domain,fine_patch,&xp,&yp,&zp,
                                          &xd,&yd,&zd,&areafine);
        }

        vt.fort_average2coarse(&mx,&my,&mbc,&meqn,qcoarse,qfine,
                               areacoarse, areafine, &igrid,
                               &gparms->manifold);

    }
}


#ifdef __cplusplus
#if 0
{
#endif
}
#endif
