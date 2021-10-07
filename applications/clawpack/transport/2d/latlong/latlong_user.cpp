/*
Copyright (c) 2012-2021 Carsten Burstedde, Donna Calhoun
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

#include "latlong_user.h"

static
void latlong_patch_setup(fclaw2d_global_t *glob,
                         fclaw2d_patch_t *patch,
                         int blockno,
                         int patchno)
{
    const user_options_t* user = latlong_get_options(glob);
    transport_patch_setup_manifold(glob,patch,blockno,patchno,
                                   user->claw_version);
}

void latlong_link_solvers(fclaw2d_global_t *glob)
{
    fclaw2d_patch_vtable_t *patch_vt = fclaw2d_patch_vt();
    patch_vt->setup = &latlong_patch_setup;

    const user_options_t   *user = latlong_get_options(glob);
    if (user->claw_version == 4)
    {
        fc2d_clawpack46_vtable_t *claw46_vt = fc2d_clawpack46_vt();
        claw46_vt->fort_setprob     = SETPROB;
        claw46_vt->fort_qinit       = CLAWPACK46_QINIT;

	 fc2d_clawpack46_options_t *claw46_opt = fc2d_clawpack46_get_options(glob);
	/* Solve conservative equation using cell-centered velocity */
        /* Fwaves give best accuracy */
        claw46_opt->use_fwaves = 1;
        claw46_vt->fort_rpn2fw = CLAWPACK46_RPN2CONS_FW_MANIFOLD;

        /* Transverse solver : Conservative form */
        claw46_vt->fort_rpt2fw  = &CLAWPACK46_RPT2CONS_MANIFOLD;

        /* Flux function (for conservative fix) */
	claw46_vt->fort_rpn2_cons = &RPN2CONS_UPDATE_MANIFOLD;
    }
    else if (user->claw_version == 5)
    {





    }
}
