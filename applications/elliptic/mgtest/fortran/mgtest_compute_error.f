      subroutine mgtest_compute_error(blockno, mx,my,mbc,meqn,
     &      dx,dy,xlower,ylower,t,q,error,soln)
      implicit none

      integer mx,my,mbc,meqn, blockno
      double precision dx, dy, xlower, ylower, t
      double precision q(1-mbc:mx+mbc,1-mbc:my+mbc,meqn)
      double precision error(1-mbc:mx+mbc,1-mbc:my+mbc,meqn)
      double precision soln(1-mbc:mx+mbc,1-mbc:my+mbc,meqn)

      integer*8 cont, get_context

      integer i,j,m
      double precision xc,yc, mgtest_qexact
      double precision xc1, yc1, zc1

      cont = get_context()

c     # Assume a single field variable only
      do j = 1,my
         yc = ylower + (j-0.5)*dy
         do i = 1,mx
            xc = xlower + (i-0.5)*dx

            soln(i,j,1) = mgtest_qexact(xc1,yc1)
            error(i,j,1) = q(i,j,1) - soln(i,j,1)
         enddo
      enddo


      end
