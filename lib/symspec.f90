subroutine symspec(k,ntrperiod,nsps,ndiskdat,nb,nbslider,pxdb,s,f0a,df3,    &
     ihsym,nzap,slimit,lstrong)

! Input:
!  k         pointer to the most recent new data
!  ntrperiod T/R sequence length, minutes
!  nsps      samples per symbol (12000 Hz)
!  ndiskdat  0/1 to indicate if data from disk
!  nb        0/1 status of noise blanker (off/on)
!  nbslider  NB setting, 0-100

! Output:
!  pxdb      power (0-60 dB)
!  s         spectrum for waterfall display
!  ihsym     index number of this half-symbol (1-322)
!  nzap      number of samples zero'ed by noise blanker
!  slimit    NB scale adjustment
!  lstrong   true if strong signal at this freq

  parameter (NMAX=1800*12000)        !Total sample intervals per 30 minutes
  parameter (NDMAX=1800*1500)        !Sample intervals at 1500 Hz rate
  parameter (NSMAX=22000)            !Max length of saved spectra
  parameter (NFFT1=1024)
  parameter (NFFT2=1024,NFFT2A=NFFT2/8)
  parameter (MAXFFT3=32768)
  real*4 s(NSMAX),w(NFFT1),w3(MAXFFT3)
  real*4 x0(NFFT1),x1(NFFT1)
  real*4 x2(NFFT1+105)
  real*4 xx(NMAX)
  real*4 ssum(NSMAX)
  complex cx(0:MAXFFT3-1)
  logical*1 lstrong(0:1023)               !Should be (0:512)
  integer*2 id2
  complex c0
  common/jt9com/id2(NMAX),ss(184,NSMAX),savg(NSMAX),c0(NDMAX),     &
       nutc,npts8,junk(20)
  data rms/999.0/,k0/99999999/,ntrperiod0/0/,nfft3z/0/
  save

  if(ntrperiod.eq.1)  nfft3=1024
  if(ntrperiod.eq.2)  nfft3=2048
  if(ntrperiod.eq.5)  nfft3=6144
  if(ntrperiod.eq.10) nfft3=12288
  if(ntrperiod.eq.30) nfft3=32768

  jstep=nsps/16
  if(k.gt.NMAX) go to 999
  if(k.lt.nfft3) then
     ihsym=0
     go to 999                                 !Wait for enough samples to start
  endif
  if(nfft3.ne.nfft3z) then
     pi=4.0*atan(1.0)
     do i=1,nfft3
        w3(i)=2.0*(sin(i*pi/nfft3))**2             !Window for nfft3
     enddo
     nfft3z=nfft3
  endif

  if(k.lt.k0) then
     ja=-3*jstep
     ssum=0.
     ihsym=0
     k1=0
     k8=0
     x2=0.
     if(ndiskdat.eq.0) id2(k+1:60*ntrperiod*12000)=0
  endif
  k0=k
 
  nzap=0
  sigmas=1.0*(10.0**(0.01*nbslider)) + 0.7
  peaklimit=sigmas*max(10.0,rms)
  faclim=3.0
  px=0.
  df2=12000.0/NFFT2

  nwindow=2
!  nwindow=0                                    !### No windowing ###
  kstep1=NFFT1
  if(nwindow.ne.0) kstep1=NFFT1/2
  fac=2.0/NFFT1
  nblks=(k-k1)/kstep1
  do nblk=1,nblks
     j=k1+1
     do i=1,NFFT1
        x0(i)=id2(k1+i)
     enddo
     call timf2(x0,k,NFFT1,nwindow,nb,peaklimit,faclim,x1,   &
          slimit,lstrong,px,nzap)

! Mix at 1500 Hz, lowpass at +/-750 Hz, and downsample to 1500 Hz complex.
     x2(106:105+kstep1)=x1(1:kstep1)
     call fil3(x2,kstep1+105,c0(k8+1),n2)
     x2(1:105)=x1(kstep1-104:kstep1)   !Save 105 trailing samples
     k1=k1+kstep1
     k8=k8+kstep1/8
  enddo

  npts8=k8
  ja=ja+jstep                         !Index of first sample
  nsum=nblks*kstep1 - nzap
!###
!  if(nzap/178.lt.50 .and. (ndiskdat.eq.0 .or. ihsym.lt.280)) then
  if(nsum.le.0) nsum=1
  rms=sqrt(px/nsum)
!  endif
  pxdb=0.
  if(rms.gt.0.0) pxdb=20.0*log10(rms)
  if(pxdb.gt.60.0) pxdb=60.0
!###
!  if(ja.lt.0 .or. npts8.lt.ja+nfft3) go to 999

  if(ja.gt.0) then
     do i=0,nfft3-1                      !Copy data into cx
        cx(i)=c0(ja+i+1)
     enddo

     ihsym=ihsym+1
     cx(0:nfft3-1)=w3(1:nfft3)*cx(0:nfft3-1)  !Apply window w3
     call four2a(cx,nfft3,1,1,1)           !Third forward FFT (X)

     n=min(184,ihsym)
     df3=1500.0/nfft3
     i0=nint(-500.0/df3)
     iz=min(NSMAX,nint(1000.0/df3))
     fac=(1.0/nfft3)**2
     do i=1,iz
        j=i0+i-1
        if(j.lt.0) j=j+nfft3
        sx=fac*(real(cx(j))**2 + aimag(cx(j))**2)
        ss(n,i)=sx
        ssum(i)=ssum(i) + sx
        s(i)=sx
     enddo
  endif

999 continue


  call pctile(s,iz,50,xmed0)
  s(1:iz)=s(1:iz)/xmed0
  call pctile(ssum,iz,50,xmed1)
  savg(1:iz)=ssum(1:iz)/xmed1

!  if(ihsym.eq.160) then
!     rewind 71
!     do i=1,iz
!        write(71,3003) 1000+i*df3,savg(i)
!3003    format(2f12.3)
!     enddo
!     flush(71)
!  endif

  return
end subroutine symspec
