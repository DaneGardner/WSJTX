subroutine sync9(ss,nzhsym,lag1,lag2,ia,ib,ccfred,ipkbest)

  parameter (NSMAX=22000)            !Max length of saved spectra
  real ss(184,NSMAX)
  real ss1(184)
  real ccfred(NSMAX)
  include 'jt9sync.f90'

  ipk=0
  ipkbest=0
  sbest=0.
  ccfred=0.

  do i=ia,ib                         !Loop over freq range
     ss1=ss(1:184,i)
     call pctile(ss1,nzhsym,50,xmed)
     ss1=ss1/xmed - 1.0
     do j=1,nzhsym
        if(ss1(j).gt.3.0) ss1(j)=5.0
     enddo

     smax=0.
     do lag=lag1,lag2                !DT = 2.5 to 5.0 s
        sum=0.
        do j=1,16                    !Sum over 16 sync symbols
           k=ii2(j) + lag
           if(k.ge.1 .and. k.le.nzhsym) sum=sum + ss1(k)
        enddo
        if(sum.gt.smax) then
           smax=sum
           ipk=i 
        endif
     enddo
     ccfred(i)=smax                        !Best at this freq, over all lags
     if(smax.gt.sbest) then
        sbest=smax
        ipkbest=ipk
     endif
  enddo

  call pctile(ccfred(ia),ib-ia+1,50,xmed)
  if(xmed.le.0.0) xmed=1.0
  ccfred=1.33*ccfred/xmed

  return
end subroutine sync9
