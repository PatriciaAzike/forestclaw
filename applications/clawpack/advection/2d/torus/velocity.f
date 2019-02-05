c     # ------------------------------------------------------------
c     # Compute edge centered and cell-centered velocity fields
c     # 
c     # Edge : Compute velocity from a stream function.  
c     # This defines the average velocity at each edge
c     # 
c     #      u = curl \Psi  (div u = 0)
c     # 
c     # Center : Defines cell-centered velocities from a streamfunction
c     # 
c     #      u = n cross grad \Psi  (div u = 0)
c     # 
c     # or using basis functions
c     # 
c     #      u = u1*tau1 + u2*tau2   (div u might not be zero)
c     # 
c     # NOTE: All arguments to routines here should be in computational 
c     # coordinates (xi,eta) in [0,1]x[0,1].  Mapping from brick domains
c     # to [0,1]x[0,1] should be done from calling routines.
c     # ------------------------------------------------------------


c     # ------------------------------------------------------------
c     # Edge : u = \nabla cross \Psi - curl \Psi
c     # ------------------------------------------------------------
      subroutine torus_edge_velocity(x1,y1,x2,y2,ds,vn)
      implicit none

      double precision x1,y1,x2,y2, ds, vn, torus_psi

      vn = (torus_psi(x1,y1) - torus_psi(x2,y2))/ds

      end

c     # ------------------------------------------------------------
c     # Center : u = n cross grad \Psi   (div u = 0)
c     # 
c     # Center : u = u1*tau1 + u2*tau2   (div u might not be zero)
c     # ------------------------------------------------------------
      subroutine torus_center_velocity(x,y,vel)
      implicit none

      double precision x,y,vel(3)

      double precision t1(3), t2(3)
      double precision t1inv(3), t2inv(3)
      double precision nvec(3), gradpsi(3), sv
      logical use_stream

      double precision p, px, py
      double precision u(2), uderivs(2)

      double precision pi, pi2
      common /compi/ pi, pi2

      integer example
      common /example_comm/ example  

      integer k

      use_stream = .false.

      if (example .eq. 0 .and. use_stream) then
c         # Divergence free velocity field : u = n cross \Psi  
          call torus_psi_derivs(x,y,p,px,py)

          call torus_contravariant_basis(x,y, t1inv,t2inv)


          do k = 1,3
              gradpsi(k) = px*t1inv(k) + py*t2inv(k)
          end do

c         # Get surface normal
c          call torus_covariant_basis(a,b,t1,t2)

c         Outward directed normal          
          call torus_cross(t2inv,t1inv,nvec,sv)

c         # Normalize surface normal
          do k = 1,3
              nvec(k) = nvec(k)/sv
          end do

c         # v = nvec x grad \Psi
          call torus_cross(nvec,gradpsi,vel,sv)
      else
c         # Vector field defined as u1*tau1 + u2*tau2    

          call torus_covariant_basis(x, y, t1,t2)
          call torus_velocity_components(x,y,u)

          do k = 1,3
              vel(k) = u(1)*t1(k) + u(2)*t2(k)
          enddo
      endif
        
      end

      subroutine torus_covariant_basis(x,y,t1,t2)
      implicit none

      double precision x,y,t1(3),t2(3)
      double precision t(3,2),tinv(3,2), tderivs(3,2,2)
      integer flag, k

c     # Compute covariant derivatives only
      flag = 1
      call torus_basis_complete(x,y, t, tinv, tderivs, flag)

      do k = 1,3
          t1(k) = t(k,1)
          t2(k) = t(k,2)
      enddo
c      write(6,*) t1(1), t1(2), t1(3)

      end


      subroutine torus_contravariant_basis(x,y,t1inv,t2inv)
      implicit none

      double precision x,y
      double precision t1inv(3), t2inv(3)
      double precision t(3,2), tinv(3,2), tderivs(3,2,2)
      integer k, flag


      flag = 3
      call torus_basis_complete(x,y, t, tinv,tderivs, flag)
                                          

      do k = 1,3
          t1inv(k) = tinv(k,1)
          t2inv(k) = tinv(k,2)
      end do

      end

      subroutine torus_christoffel_sym(x,y,g) 
      implicit none

      double precision x, y
      double precision g(2,2,2)

      double precision torus_dot 

      double precision pi, pi2
      common /compi/ pi, pi2

      double precision t(3,2), tinv(3,2), tderivs(3,2,2)
      double precision s(3,2), tij(3), sk(3)

      integer i,j,k, m, flag

c     # Compute covariant and derivatives

      flag = 7
      call torus_basis_complete(x,y, t, tinv,tderivs, flag)
                                          

      do k = 1,3
          s(k,1) = tinv(k,1)
          s(k,2) = tinv(k,2)
      end do

      do i = 1,2
          do j = 1,2
              do k = 1,2
                  do m = 1,3
                      tij(m) = tderivs(m,i,j)
                      sk(m) = s(m,k) 
                  enddo
                  g(i,j,k) = torus_dot(tij,sk)
              enddo
          enddo
      enddo

      end

      double precision function torus_divergence(x,y)
      implicit none

      double precision x,y

      double precision u(2), uderivs(4), g(2,2,2)
      double precision D11, D22

c     # Get g(i,j,k), g = \Gamma(i,j,k)
      call torus_velocity_derivs(x,y,u,uderivs)
      call torus_christoffel_sym(x,y,g) 

      D11 = uderivs(1) + u(1)*g(1,1,1) + u(2)*g(1,1,2)
      D22 = uderivs(2) + u(1)*g(2,2,1) + u(2)*g(2,2,2)
      
      torus_divergence = D11 + D22
      end


c     # ----------------------------------------------------------------
c     # RHS functions for ODE solver DOPRI5  (original ode45)
c     #        -- divfree field
c     #        -- field with divergence
c     # ----------------------------------------------------------------

      subroutine torus_rhs_divfree(n,t,sigma,f,rpar,ipar)
      implicit none

      integer n, ipar
      double precision t, sigma(n), f(n), rpar


      integer m, i
      double precision x,y, q, u1,u2
      double precision p, px, py
      double precision t1(3), t2(3), t1xt2(3), w

      x = sigma(1)
      y = sigma(2)

      call torus_covariant_basis(x,y,t1,t2)
      call torus_psi_derivs(x,y,p, px,py)

c     # Compute u dot grad q in computational coordinates
      call torus_cross(t1,t2,t1xt2,w);

c     # Evolve contravariant components of velocity field          
      u1 = -py/w
      u2 = px/w

c     # This is a clockwise velocity field ? 
      f(1) = u1
      f(2) = u2

      end

      subroutine torus_rhs_nondivfree(n,t,sigma,f,rpar,ipar)
      implicit none

      integer n, ipar
      double precision t, sigma(n), f(n), rpar


      double precision x,y, q
      double precision u(2)
      double precision divu, torus_divergence

c     # Track evolution of these three quantities
      x = sigma(1)
      y = sigma(2)
      q = sigma(3)

      call torus_velocity_components(x,y,u)

      divu = torus_divergence(x,y)

      f(1) = u(1)
      f(2) = u(2)
      f(3) = -divu*q   !! Non-conservative case

      end

c     # ----------------------------------------------------------------
c     # Utility functions
c     # ----------------------------------------------------------------

      subroutine torus_cross(u,v,uxv,w)
      implicit none

      double precision u(3),v(3),uxv(3),w, torus_dot
      integer k

      uxv(1) =   u(2)*v(3) - u(3)*v(2)
      uxv(2) = -(u(1)*v(3) - u(3)*v(1))
      uxv(3) =   u(1)*v(2) - u(2)*v(1)

      w = torus_dot(uxv,uxv)
      w = sqrt(w)      

      end

      double precision function torus_dot(u,v)
      implicit none

      double precision u(3),v(3)

      torus_dot = u(1)*v(1) + u(2)*v(2) + u(3)*v(3)

      end





