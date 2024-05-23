subroutine qinit(meqn,mbc,mx,xlower,dx,q,maux,aux)
    implicit none

    integer meqn, mbc, mx, maux
    double precision xlower,dx
    double precision q(meqn,1-mbc:mx+mbc)
    double precision aux(maux,1-mbc:mx+mbc)

    double precision pi, pi2
    common /compi/ pi, pi2

    double precision hin, hout
    common /swe_initcond_parms4/  hin,hout

    double precision Px, Py, Pz, theta_ridge, theta_wave, & 
                        ampl, alpha, bathy(2), speed
    common /comm_ridge/ Px, Py, Pz, theta_ridge, theta_wave, & 
                        ampl, alpha, bathy, speed

    integer i
    double precision xcell, theta, q1, sea_level, deg2rad

    !!xlower_comm = xlower !! + pi/2.d0 ! needed in rp1
    !!dx_comm = dx

    sea_level = 0

    deg2rad = pi/180
    do i = 1-mbc,mx+mbc
         xcell = xlower + (i-0.5d0)*dx

         q(1,i) = sea_level-aux(1,i)
         q(2,i) = 0.d0

         theta = deg2rad*xcell

         q1 = dexp(-alpha*(theta-theta_wave)**2)
         if (q1 .gt. 1d-20) then
            q(1,i) = q(1,i) + ampl*q1
            q(2,i) = -speed*ampl*q1
         endif
    end do
    !!write(6,*) 'Stopping in qinit'
    !!stop
    return
end
