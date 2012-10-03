subroutine peakdt9(c0,npts8,nsps8,istart,foffset,idtpk)

  complex c0(0:npts8-1)
  complex zsum
  integer ii(16)                       !Locations of sync symbols
  data ii/1,6,11,16,21,26,31,39,45,51,57,63,69,75,81,85/

  twopi=8.0*atan(1.0)
  smax=0.

  f0=foffset
  dphi=twopi*f0/1500.0
  do idt=-5,5
     i0=istart + 0.0625*nsps8*idt
    sum=0.
     do j=1,16
        i1=(ii(j)-1)*nsps8 + i0
        phi=0.
        zsum=0.
        do i=i1,i1+nsps8-1
           if(i.lt.0 .or. i.gt.npts8-1) cycle
           phi=phi + dphi
           zsum=zsum + c0(i) * cmplx(cos(phi),-sin(phi))
        enddo
        sum=sum + real(zsum)**2 + aimag(zsum)**2
     enddo
     if(sum.gt.smax) then
        idtpk=idt
        smax=sum
     endif
  enddo

  return
end subroutine peakdt9
