## Makefile.am in applications/clawpack/advection/3d/filament

bin_PROGRAMS += applications/clawpack/advection/3d/filament/filament

TESTS += applications/clawpack/advection/3d/filament/regressions.sh

applications_clawpack_advection_3d_filament_filament_SOURCES = \
	applications/clawpack/advection/3d/filament/filament.cpp \
	applications/clawpack/advection/3d/filament/filament_options.c \
	applications/clawpack/advection/3d/filament/filament_user.cpp \
	applications/clawpack/advection/3d/filament/filament_map_extrude.c \
	applications/clawpack/advection/2d/all/fclaw2d_map_cart.c \
	applications/clawpack/advection/2d/all/fclaw2d_map_fivepatch.c \
	applications/clawpack/advection/2d/all/fclaw2d_map_bilinear.c \
	applications/clawpack/advection/3d/filament/user_exceeds_th.f90 \
	applications/clawpack/advection/3d/filament/psi.f \
	applications/clawpack/advection/3d/filament/fdisc.f \
	applications/clawpack/advection/3d/filament/setprob.f \
	applications/clawpack/advection/3d/filament/user_4.6/qinit.f90 \
	applications/clawpack/advection/3d/filament/user_4.6/setaux.f90 \
	applications/clawpack/advection/3d/all/advection_patch_setup_manifold.c \
	applications/clawpack/advection/3d/all/claw3_setaux_manifold.f90 \
	applications/clawpack/advection/3d/all/rpn3_vc_advection.f90 \
	applications/clawpack/advection/3d/all/rpt3_vc_advection.f90 \
	applications/clawpack/advection/3d/all/rptt3_vc_advection.f90

EXTRA_DIST += \
	applications/clawpack/advection/3d/filament/fclaw_options.ini \
	applications/clawpack/advection/3d/filament/mapping.ini \
	applications/clawpack/advection/3d/filament/regression.ini \
	applications/clawpack/advection/3d/filament/regression_map.ini \
	applications/clawpack/advection/3d/filament/afterframe.m \
	applications/clawpack/advection/3d/filament/setplot3.m  \
	applications/clawpack/advection/3d/filament/mapc2p.m \
	applications/clawpack/advection/3d/filament/read_vars.m

applications_clawpack_advection_3d_filament_filament_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        $(FCLAW_CLAWPACK3_CPPFLAGS) \
        $(FCLAW_CLAWPATCH_CPPFLAGS)
	    

# applications_clawpack_advection_3d_filament_filament_CPPFLAGS += -D MPI_DEBUG


applications_clawpack_advection_3d_filament_filament_LDADD = \
        $(LDADD) \
        $(FCLAW_CLAWPACK3_LDADD) \
        $(FCLAW_CLAWPATCH_LDADD) \
        $(FCLAW_LDADD)