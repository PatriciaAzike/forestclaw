! ::::::::::::::::::::: flag2refine ::::::::::::::::::::::::::::::::::
!
!> \callgraph
!! \callergraph
!! User routine to control flagging of points for refinement.
!!
!! Default version computes spatial difference dq in each direction and
!! for each component of q and flags any point where this is greater than
!! the tolerance tolsp.  
!! This is consistent with what the routine errsp did in
!!  earlier versions of amrclaw (4.2 and before).
!!
!! This routine can be copied to an application directory and modified to
!! implement some other desired refinement criterion.
!!
!! Points may also be flagged for refining based on a Richardson estimate
!! of the error, obtained by comparing solutions on the current grid and a
!! coarsened grid.  Points are flagged if the estimated error is larger than
!! the parameter tol in amr2ez.data, provided flag_richardson is .true.,
!! otherwise the coarsening and Richardson estimation is not performed!  
!! Points are flagged via Richardson in a separate routine.
!!
!! Once points are flagged via this routine and/or Richardson, the subroutine
!! flagregions is applied to check each point against the min_level and
!! max_level of refinement specified in any "region" set by the user.
!! So flags set here might be over-ruled by region constraints.
!!
!! **output**: amrflags
!!
!! \param mx number of cells in *i* direction
!! \param my number of cells in *j* direction
!! \param mbc width of ghost cell region
!! \param mbuff width of buffer region
!! \param meqn number of equations for the system
!! \param maux number of auxiliary variables
!! \param xlower x-coordinate of left physical boundary
!! \param ylower y-coordinate of lower physical boundary
!! \param dx spacing in *i* direction
!! \param dy spacing in *j* direction
!! \param t simulation time on this grid
!! \param level AMR level of this grid
!! \param tolsp tolerance specified by user in input file amr.data, used in default
!!         version of this routine as a tolerance for spatial differences
!! \param q grid values including ghost cells (bndry vals at specified
!!          time have already been set, so can use ghost cell values too)
!! \param aux auxiliary array on this grid patch
!! \param amrflags array to be flagged with either the value **DONTFLAG** or **DOFLAG** for each cell. 
!!        It is enlarged from grid size to include buffer regions around the grid. 
!! \param DONTFLAG value to be assigned to amrflags for cells that need no refinement
!! \param DOFLAG value to be assigned to amrflags for cells that do need refinement
!
! ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

subroutine flag2refine2(mx,my,mbc,mbuff,meqn,maux,xlower,ylower,dx,dy,t,level, &
                            tolsp,q,aux,amrflags)

    use regions_module
    use amr_module, only : DOFLAG, UNSET

    implicit none

    ! Subroutine arguments
    integer, intent(in) :: mx,my,mbc,meqn,maux,level,mbuff
    real(kind=8), intent(in) :: xlower,ylower,dx,dy,t,tolsp
    
    real(kind=8), intent(in) :: q(meqn,1-mbc:mx+mbc,1-mbc:my+mbc)
    real(kind=8), intent(in) :: aux(maux,1-mbc:mx+mbc,1-mbc:my+mbc)
    
    ! Flagging
    real(kind=8),intent(inout) :: amrflags(1-mbuff:mx+mbuff,1-mbuff:my+mbuff)
    
    logical :: allowflag
    external allowflag

    DOUBLE precision pi, pi2
    COMMON /compi/ pi, pi2

    DOUBLE PRECISION alpha, beta, theta_range(2), phi_range(2)
    COMMON /torus_comm/ alpha, beta, theta_range, phi_range

    INTEGER refine_pattern
    COMMON /refine_comm/ refine_pattern


    ! Locals
    integer :: i,j,m
    real(kind=8) :: x_c,y_c,x_low,y_low,x_hi,y_hi
    real(kind=8) :: dqi(meqn), dqj(meqn), dq(meqn)
    REAL(kind=8) :: qmin, qmax, r, ravg, rw, th
    LOGICAL constant_theta, constant_r

    double precision xc,yc
    logical refine


    ! Don't initialize flags, since they were already 
    ! flagged by flagregions2
    ! amrflags = DONTFLAG
    
    ! Loop over interior points on this grid
    ! (i,j) grid cell is [x_low,x_hi] x [y_low,y_hi], cell center at (x_c,y_c)
    ! This information is not needed for the default flagging based on
    ! undivided differences, but might be needed in a user's version.
    ! Note that if you want to refine only in certain space-time regions,
    ! it may be easiest to use the "regions" feature. The flags set here or
    ! in the Richardson error estimator are modifing the flags set by
    ! min_level and max_level specified in any regions.

    y_loop: do j=1,my
        !y_low = ylower + (j - 1) * dy
        !y_c = ylower + (j - 0.5d0) * dy
        !y_hi = ylower + j * dy
        
        x_loop: do i = 1,mx
            !x_low = xlower + (i - 1) * dx
            !x_c = xlower + (i - 0.5d0) * dx
            !x_hi = xlower + i * dx

            ! -----------------------------------------------------------------
            ! Only check undivided differences if flag hasn't been set yet. 
            ! If flag == DONTFLAG then refinement is forbidden by a region, 
            ! if flag == DOFLAG checking is not needed

            if(amrflags(i,j) == UNSET) then
                if (refine_pattern .eq. -1) then
                    amrflags(i,j) = DOFLAG
                else if (refine_pattern .eq. 0) then
                    refine = (q(1,i,j) .gt.  tolsp) .and. (q(1,i,j) .lt. 1-tolsp)

                    if (refine) then
                        amrflags(i,j) = DOFLAG
                    endif
                else
                    xc = xlower + (i-0.5)*dx
                    yc = ylower + (j-0.5)*dy
                    th = pi2*(theta_range(1) + (theta_range(2)-theta_range(1))*xc)
                    constant_theta = th .gt. pi/2
                    constant_r = abs(yc-0.5) .gt. 0.25
                    if (refine_pattern .eq. 1 .and. constant_theta) then 
                        amrflags(i,j) = DOFLAG
                        cycle x_loop
                    elseif (refine_pattern .eq. 2 .and. constant_r) then
                        amrflags(i,j) = DOFLAG
                        cycle x_loop
                    endif
                endif
            endif
        enddo x_loop
    enddo y_loop
   
end subroutine flag2refine2
