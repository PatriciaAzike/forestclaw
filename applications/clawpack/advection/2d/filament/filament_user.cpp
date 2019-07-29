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

#include "filament_user.h"

#include <fclaw2d_include_all.h>

#include <fclaw2d_clawpatch.h>

#include <fc2d_clawpack46.h>
#include <fc2d_clawpack5.h>

#include "../all/advection_user_fort.h"


void filament_link_solvers(fclaw2d_global_t *glob)
{
    const user_options_t* user = filament_get_options(glob);
    const fclaw_options_t* fclaw_opt = fclaw2d_get_options(glob);

    fclaw2d_patch_vtable_t *patch_vt = fclaw2d_patch_vt();
    fclaw2d_clawpatch_vtable_t *clawpatch_vt = fclaw2d_clawpatch_vt();
    
    if (user->claw_version == 4)
    {
        fc2d_clawpack46_vtable_t *clawpack46_vt = fc2d_clawpack46_vt();

        clawpack46_vt->fort_setprob   = &SETPROB;
        clawpack46_vt->fort_qinit     = &CLAWPACK46_QINIT;

        patch_vt->setup = fc2d_clawpack46_setaux;

        if (fclaw_opt->manifold)
        {
            patch_vt->setup           = &filament_patch_setup_manifold;

            clawpack46_vt->fort_rpn2  = &CLAWPACK46_RPN2ADV_MANIFOLD;
            clawpack46_vt->fort_rpt2  = &CLAWPACK46_RPT2ADV_MANIFOLD;

            if (user->example == 2)
            {
                /* Avoid tagging block corners in 5 patch example*/
                clawpatch_vt->fort_tag4refinement = &CLAWPACK46_TAG4REFINEMENT;
                clawpatch_vt->fort_tag4coarsening = &CLAWPACK46_TAG4COARSENING;
            }
        }
        else
        {
            clawpack46_vt->fort_setaux    = &CLAWPACK46_SETAUX;  /* Used in non-manifold case */
            clawpack46_vt->fort_rpn2      = &CLAWPACK46_RPN2ADV;
            clawpack46_vt->fort_rpt2      = &CLAWPACK46_RPT2ADV;
        }
    }
    else if (user->claw_version == 5)
    {
        fc2d_clawpack5_vtable_t *claw5_vt = fc2d_clawpack5_vt();

        claw5_vt->fort_setprob   = &SETPROB;
        claw5_vt->fort_qinit     = &CLAWPACK5_QINIT;
        
        if (fclaw_opt->manifold)
        {
            patch_vt->setup       = filament_patch_setup_manifold;
            claw5_vt->fort_rpn2   = CLAWPACK5_RPN2ADV_MANIFOLD;
            claw5_vt->fort_rpt2   = CLAWPACK5_RPT2ADV_MANIFOLD;

            if (user->example == 2)
            {
                /* Avoid tagging block corners in 5 patch example*/
                clawpatch_vt->fort_tag4refinement = CLAWPACK5_TAG4REFINEMENT;
                clawpatch_vt->fort_tag4coarsening = CLAWPACK5_TAG4COARSENING;
            }
        }
        else
        {
            claw5_vt->fort_setaux    = CLAWPACK5_SETAUX;   /* Used in non-manifold case */
            claw5_vt->fort_rpn2      = CLAWPACK5_RPN2ADV;
            claw5_vt->fort_rpt2      = CLAWPACK5_RPT2ADV;
        }
    }
}

void filament_patch_setup_manifold(fclaw2d_global_t *glob,
                                   fclaw2d_patch_t *this_patch,
                                   int this_block_idx,
                                   int this_patch_idx)
{
    const user_options_t* user = filament_get_options(glob);

    int mx,my,mbc,maux;
    double xlower,ylower,dx,dy;
    double *aux,*xd,*yd,*zd,*area;
    double *xp,*yp,*zp;

    fclaw2d_clawpatch_grid_data(glob,this_patch,&mx,&my,&mbc,
                                &xlower,&ylower,&dx,&dy);

    fclaw2d_clawpatch_metric_data(glob,this_patch,&xp,&yp,&zp,
                                  &xd,&yd,&zd,&area);
    
    fclaw2d_clawpatch_aux_data(glob,this_patch,&aux,&maux);
    if (user->claw_version == 4)
    {
        USER46_SETAUX_MANIFOLD(&mbc,&mx,&my,&xlower,&ylower,
                               &dx,&dy,&maux,aux,&this_block_idx,
                               xd,yd,zd,area);
    }
    else if (user->claw_version == 5)
    {
        USER5_SETAUX_MANIFOLD(&mbc,&mx,&my,&xlower,&ylower,
                              &dx,&dy,&maux,aux,&this_block_idx,
                              xd,yd,zd,area);
    }
}
