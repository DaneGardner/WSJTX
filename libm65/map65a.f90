subroutine map65a(dd,ss,savg,newdat,nutc,fcenter,ntol,idphi,nfa,nfb,        &
     mousedf,mousefqso,nagain,ndecdone,ndiskdat,nfshift,ndphi,              &
     nfcal,nkeep,mcall3b,nsave,nxant,rmsdd,mycall,mygrid,                    &
     neme,ndepth,hiscall,hisgrid,nhsym,nfsample,nxpol,mode65)

!  Processes timf2 data from Linrad to find and decode JT65 signals.

  parameter (MAXMSG=1000)            !Size of decoded message list
  parameter (NSMAX=60*96000)
  parameter (NFFT=32768)
  real dd(4,NSMAX)
  real*4 ss(4,322,NFFT),savg(4,NFFT)
  real tavg(-50:50)                  !Temp for finding local base level
  real base(4)                       !Local basel level at 4 pol'ns
  real tmp (200)                     !Temp storage for pctile sorting
  real sig(MAXMSG,30)                !Parameters of detected signals
  real a(5)
  real*8 fcenter
  character*22 msg(MAXMSG)
  character*3 shmsg0(4)
  character mycall*12,hiscall*12,mygrid*6,hisgrid*6,grid*6,cp*1
  integer indx(MAXMSG),nsiz(MAXMSG)
  logical done(MAXMSG)
  logical xpol
  character decoded*22,blank*22
  real short(3,NFFT)                 !SNR dt ipol for potential shorthands
  real qphi(12)
  common/c3com/ mcall3a
  common/testcom/ifreq
  
  data blank/'                      '/
  data shmsg0/'ATT','RO ','RRR','73 '/
  data nfile/0/,nutc0/-999/,nid/0/,ip000/1/,ip001/1/,mousefqso0/-999/
  save

  mcall3a=mcall3b
  mousefqso0=mousefqso
  xpol=(nxpol.ne.0)
  if(.not.xpol) ndphi=0

!### Should use AppDir! ###
!  open(23,file='release/CALL3.TXT',status='unknown')
  open(23,file='CALL3.TXT',status='unknown')

  if(nutc.ne.nutc0) nfile=nfile+1
  nutc0=nutc
  df=96000.0/NFFT                     !df = 96000/NFFT = 2.930 Hz
  if(nfsample.eq.95238) df=95238.1/NFFT
  ftol=0.010                          !Frequency tolerance (kHz)
  dphi=idphi/57.2957795
  foffset=0.001*(1270 + nfcal)              !Offset from sync tone, plus CAL
  fqso=mousefqso + foffset - 0.5*(nfa+nfb) + nfshift !fqso at baseband (khz)
  iloop=0

2  if(ndphi.eq.1) dphi=30*iloop/57.2957795

  do nqd=1,0,-1
     if(nqd.eq.1) then                     !Quick decode, at fQSO
        fa=1000.0*(fqso+0.001*mousedf) - ntol
        fb=1000.0*(fqso+0.001*mousedf) + ntol + 4*53.8330078
     else                                  !Wideband decode at all freqs
        fa=-1000*0.5*(nfb-nfa) + 1000*nfshift
        fb= 1000*0.5*(nfb-nfa) + 1000*nfshift
     endif
     ia=nint(fa/df) + 16385
     ib=nint(fb/df) + 16385
     ia=max(51,ia)
     ib=min(32768-51,ib)

     km=0
     nkm=1
     nz=n/8
     freq0=-999.
     sync10=-999.
     fshort0=-999.
     syncshort0=-999.
     ntry=0
     short=0.                                 !Zero the whole short array
     jpz=1
     if(xpol) jpz=4

     do i=ia,ib                               !Search over freq range
        freq=0.001*(i-16385)*df
!  Find the local base level for each polarization; update every 10 bins.
        if(mod(i-ia,10).eq.0) then
           do jp=1,jpz
              do ii=-50,50
                 iii=i+ii
                 if(iii.ge.1 .and. iii.le.32768) then
                    tavg(ii)=savg(jp,iii)
                 else
                    write(13,*) ,'Error in iii:',iii,ia,ib,fa,fb
                    flush(13)
                    go to 999
                 endif
              enddo
              call pctile(tavg,tmp,101,50,base(jp))
           enddo
        endif

!  Find max signal at this frequency
        smax=0.
        do jp=1,jpz
           if(savg(jp,i)/base(jp).gt.smax) then
              smax=savg(jp,i)/base(jp)
              jpmax=jp
           endif
        enddo

        if(smax.gt.1.1) then

!  Look for JT65 sync patterns and shorthand square-wave patterns.
           call timer('ccf65   ',0)
!           ssmax=4.0*(rmsdd/22.5)**2
           ssmax=savg(jpmax,i)
           call ccf65(ss(1,1,i),nhsym,ssmax,sync1,ipol,jpz,dt,flipk,     &
                syncshort,snr2,ipol2,dt2)
           call timer('ccf65   ',1)

! ########################### Search for Shorthand Messages #################
!  Is there a shorthand tone above threshold?
           thresh0=1.0
!  Use lower thresh0 at fQSO
           if(nqd.eq.1 .and. ntol.le.100) thresh0=0.
           if(syncshort.gt.thresh0) then
! ### Do shorthand AFC here (or maybe after finding a pair?) ###
              short(1,i)=syncshort
              short(2,i)=dt2
              short(3,i)=ipol2
           
!  Check to see if lower tone of shorthand pair was found.
              do j=2,4
                 i0=i-nint(j*mode65*10.0*(11025.0/4096.0)/df)
!  Should this be i0 +/- 1, or just i0?
!  Should we also insist that difference in DT be either 1.5 or -1.5 s?
                 if(short(1,i0).gt.thresh0) then
                    fshort=0.001*(i0-16385)*df
                    noffset=0
                    if(nqd.eq.1) noffset=nint(1000.0*(fshort-fqso)-mousedf)
                    if(abs(noffset).le.ntol) then
!  Keep only the best candidate within ftol.
!### NB: sync2 was not defined here!
!                       sync2=syncshort                   !### try this ???
                       if(fshort-fshort0.le.ftol .and. syncshort.gt.syncshort0 &
                            .and. nkm.eq.2) km=km-1
                       if(fshort-fshort0.gt.ftol .or.                     &
                            syncshort.gt.syncshort0) then
                          if(km.lt.MAXMSG) km=km+1
                          sig(km,1)=nfile
                          sig(km,2)=nutc
                          sig(km,3)=fshort + 0.5*(nfa+nfb)
                          sig(km,4)=syncshort
                          sig(km,5)=dt2
                          sig(km,6)=45*(ipol2-1)/57.2957795
                          sig(km,7)=0
                          sig(km,8)=snr2
                          sig(km,9)=0
                          sig(km,10)=0
!                           sig(km,11)=rms0
                          sig(km,12)=savg(ipol2,i)
                          sig(km,13)=0
                          sig(km,14)=0
                          sig(km,15)=0
                          sig(km,16)=0
!                           sig(km,17)=0
                          sig(km,18)=0
                          msg(km)=shmsg0(j)
                          fshort0=fshort
                          syncshort0=syncshort
                          nkm=2
                       endif
                    endif
                 endif
              enddo
           endif

! ########################### Search for Normal Messages ###########
!  Is sync1 above threshold?
           thresh1=1.0
!  Use lower thresh1 at fQSO
           if(nqd.eq.1 .and. ntol.le.100) thresh1=0.
           noffset=0
           if(nqd.eq.1) noffset=nint(1000.0*(freq-fqso)-mousedf)

           if(sync1.gt.thresh1 .and. abs(noffset).le.ntol) then
!  Keep only the best candidate within ftol.
!  (Am I deleting any good decodes by doing this?)
              if(freq-freq0.le.ftol .and. sync1.gt.sync10 .and.       &
                   nkm.eq.1) km=km-1
              if(freq-freq0.gt.ftol .or. sync1.gt.sync10) then
                 nflip=nint(flipk)
                 f00=(i-1)*df          !Freq of detected sync tone (0-96000 Hz)
                 ntry=ntry+1
                 if((nqd.eq.1 .and. ntry.ge.40) .or.                  &
                          (nqd.eq.0 .and. ntry.ge.400)) then
! Too many calls to decode1a!
                    write(*,*) '! Signal too strong, or suspect data?  Decoding aborted.'
                    write(13,*) 'Signal too strong, or suspect data?  Decoding aborted.'
                    call flush(13)
                    go to 999
                 endif
                 call timer('decode1a',0)
                 ifreq=i
                 ikHz=nint(freq+0.5*(nfa+nfb)-foffset)-nfshift
                 idf=nint(1000.0*(freq+0.5*(nfa+nfb)-foffset-(ikHz+nfshift)))
                 call decode1a(dd,newdat,f00,nflip,mode65,nfsample,xpol,  &
                      mycall,hiscall,hisgrid,neme,ndepth,nqd,dphi,        &
                      nutc,ikHz,idf,ipol,sync2,a,dt,pol,nkv,nhist,qual,decoded)
                 dt=dt+0.8                           !### empirical tweak
                 call timer('decode1a',1)

                 if(km.lt.MAXMSG) km=km+1
                 sig(km,1)=nfile
                 sig(km,2)=nutc
                 sig(km,3)=freq + 0.5*(nfa+nfb)
                 sig(km,4)=sync1
                 sig(km,5)=dt
                 sig(km,6)=pol
                 sig(km,7)=flipk
                 sig(km,8)=sync2
                 sig(km,9)=nkv
                 sig(km,10)=qual
!                 sig(km,11)=idphi
                 sig(km,12)=savg(ipol,i)
                 sig(km,13)=a(1)
                 sig(km,14)=a(2)
                 sig(km,15)=a(3)
                 sig(km,16)=a(4)
!                     sig(km,17)=a(5)
                 sig(km,18)=nhist
                 msg(km)=decoded
                 freq0=freq
                 sync10=sync1
                 nkm=1
              endif
           endif
        endif
!70      continue
     enddo

     if(nqd.eq.1) then
        nwrite=0
        do k=1,km
           decoded=msg(k)
           if(decoded.ne.'                      ') then
              nutc=sig(k,2)
              freq=sig(k,3)
              sync1=sig(k,4)
              dt=sig(k,5)
              npol=nint(57.2957795*sig(k,6))
              flip=sig(k,7)
              sync2=sig(k,8)
              nkv=sig(k,9)
              nqual=sig(k,10)
!              idphi=nint(sig(k,11))
              if(flip.lt.0.0) then
                 do i=22,1,-1
                    if(decoded(i:i).ne.' ') go to 8
                 enddo
                 stop 'Error in message format'
8                if(i.le.18) decoded(i+2:i+4)='OOO'
              endif
              nkHz=nint(freq-foffset)-nfshift
              mhz=fcenter                         ! ... +fadd ???
              f0=mhz+0.001*nkHz
              ndf=nint(1000.0*(freq-foffset-(nkHz+nfshift)))
              nsync1=sync1
              nsync2=nint(10.0*log10(sync2)) - 40 !### empirical ###
              if(decoded(1:4).eq.'RO  ' .or. decoded(1:4).eq.'RRR  ' .or.  &
                   decoded(1:4).eq.'73  ') nsync2=nsync2-6
              nwrite=nwrite+1
              if(nxant.ne.0) then
                 npol=npol-45
                 if(npol.lt.0) npol=npol+180
              endif

!  If Tx station's grid is in decoded message, compute optimum TxPol
              i1=index(decoded,' ')
              i2=index(decoded(i1+1:),' ') + i1
              grid='      '
              if(i2.ge.8 .and. i2.le.18) grid=decoded(i2+1:i2+4)//'mm'
              ntxpol=0
              cp=' '
              if(xpol) then
                 if(grid(1:1).ge.'A' .and. grid(1:1).le.'R' .and.           &
                      grid(2:2).ge.'A' .and. grid(2:2).le.'R' .and.         &
                      grid(3:3).ge.'0' .and. grid(3:3).le.'9' .and.         &
                      grid(4:4).ge.'0' .and. grid(4:4).le.'9') then                 
                    ntxpol=mod(npol-nint(2.0*dpol(mygrid,grid))+720,180)
                    if(nxant.eq.0) then
                       cp='H'
                       if(ntxpol.gt.45 .and. ntxpol.le.135) cp='V'
                    else
                       cp='/'
                       if(ntxpol.ge.90 .and. ntxpol.lt.180) cp='\\'
                    endif
                 endif
              endif
              
              if(ndphi.eq.0) then
                 write(*,1010) nkHz,ndf,npol,nutc,dt,nsync2,    &
                      decoded,nkv,nqual,ntxpol,cp
1010             format('!',i3,i5,i4,i5.4,f5.1,i4,2x,a22,i4,i5,i5,1x,a1)
              else
                 if(iloop.ge.1) qphi(iloop)=sig(k,10)
                 write(*,1010) nkHz,ndf,npol,nutc,dt,nsync2,    &
                      decoded,nkv,nqual,30*iloop
                 write(27,1011) 30*iloop,nkHz,ndf,npol,nutc,  &
                      dt,sync2,nkv,nqual,decoded
1011             format(i3,i4,i5,i4,i5.4,f5.1,f7.1,i3,i5,2x,a22)
              endif
           endif
        enddo

        if(nwrite.eq.0) then
           write(*,1012) mousefqso,nutc
1012       format('!',i3,9x,i5.4,' ')
        endif
   
     endif
     if(ndphi.eq.1 .and.iloop.lt.12) then
        iloop=iloop+1
        go to 2
     endif
     
     if(ndphi.eq.1 .and.iloop.eq.12) call getdphi(qphi)
     if(nagain.eq.1) go to 999
  enddo

!  Trim the list and produce a sorted index and sizes of groups.
!  (Should trimlist remove all but best SNR for given UTC and message content?)
  call trimlist(sig,km,ftol,indx,nsiz,nz)

  do i=1,km
     done(i)=.false.
  enddo
  j=0
  ilatest=-1
  do n=1,nz
     ifile0=0
     do m=1,nsiz(n)
        i=indx(j+m)
        ifile=sig(i,1)
        if(ifile.gt.ifile0 .and.msg(i).ne.blank) then
           ilatest=i
           ifile0=ifile
        endif
     enddo
     i=ilatest

     if(i.ge.1) then
        if(.not.done(i)) then
           done(i)=.true.
           nutc=sig(i,2)
           freq=sig(i,3)
           sync1=sig(i,4)
           dt=sig(i,5)
           npol=nint(57.2957795*sig(i,6))
           flip=sig(i,7)
           sync2=sig(i,8)
           nkv=sig(i,9)
           nqual=min(sig(i,10),10.0)
!                  rms0=sig(i,11)
           do k=1,5
              a(k)=sig(i,12+k)
           enddo
           nhist=sig(i,18)
           decoded=msg(i)
           
           if(flip.lt.0.0) then
              do i=22,1,-1
                 if(decoded(i:i).ne.' ') go to 10
              enddo
              stop 'Error in message format'
10            if(i.le.18) decoded(i+2:i+4)='OOO'
           endif
           mhz=fcenter                             !... +fadd ???
           nkHz=nint(freq-foffset)-nfshift
           f0=mhz+0.001*nkHz
           ndf=nint(1000.0*(freq-foffset-(nkHz+nfshift)))
           ndf0=nint(a(1))
           ndf1=nint(a(2))
           ndf2=nint(a(3))
           nsync1=sync1
           nsync2=nint(10.0*log10(sync2)) - 40 !### empirical ###
           if(decoded(1:4).eq.'RO  ' .or. decoded(1:4).eq.'RRR  ' .or.  &
                decoded(1:4).eq.'73  ') nsync2=nsync2-6
           if(nxant.ne.0) then
              npol=npol-45
              if(npol.lt.0) npol=npol+180
           endif

!  If Tx station's grid is in decoded message, compute optimum TxPol
           i1=index(decoded,' ')
           i2=index(decoded(i1+1:),' ') + i1
           grid='      '
           if(i2.ge.8 .and. i2.le.18) grid=decoded(i2+1:i2+4)//'mm'
           ntxpol=0
           cp=' '
           if(xpol) then
              if(grid(1:1).ge.'A' .and. grid(1:1).le.'R' .and.           &
                   grid(2:2).ge.'A' .and. grid(2:2).le.'R' .and.         &
                   grid(3:3).ge.'0' .and. grid(3:3).le.'9' .and.         &
                   grid(4:4).ge.'0' .and. grid(4:4).le.'9') then                 
                 ntxpol=mod(npol-nint(2.0*dpol(mygrid,grid))+720,180)
                 if(nxant.eq.0) then
                    cp='H'
                    if(ntxpol.gt.45 .and. ntxpol.le.135) cp='V'
                 else
                    cp='/'
                    if(ntxpol.ge.90 .and. ntxpol.lt.180) cp='\\'
                 endif
              endif
           endif
           write(26,1014) f0,ndf,ndf0,ndf1,ndf2,dt,npol,nsync1,       &
                nsync2,nutc,decoded,nkv,nqual,nhist,cp
           write(21,1014) f0,ndf,ndf0,ndf1,ndf2,dt,npol,nsync1,       &
                nsync2,nutc,decoded,nkv,nqual,nhist
1014       format(f8.3,i5,3i3,f5.1,i4,i3,i4,i5.4,2x,a22,3i3,1x,a1)

        endif
     endif
     j=j+nsiz(n)
  enddo
  write(26,1015) nutc
1015 format(39x,i4.4)
  call flush(21)
  call flush(26)
  call display(nkeep,ftol)
  ndecdone=2

999 close(23)
  ndphi=0
  nagain=0
  mcall3b=mcall3a

  return
end subroutine map65a
