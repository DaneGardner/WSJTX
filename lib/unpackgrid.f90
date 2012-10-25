      subroutine unpackgrid(ng,grid)

      parameter (NGBASE=180*180)
      character grid*4,grid6*6

      grid='    '
      if(ng.ge.32400) go to 10
      dlat=mod(ng,180)-90
      dlong=(ng/180)*2 - 180 + 2
      call deg2grid(dlong,dlat,grid6)
      grid=grid6(:4)
      if(grid(2:2).eq.'A' .and. grid(4:4).eq.'0') then
         i=ichar(grid(1:1))
         if(i.ge.ichar('A') .and. i.le.ichar('D')) then
            call grid2n(grid,n)
            if(n.ge.-50) write(grid,1012) n
            if(n.lt.-50) write(grid,1022) n+20
         endif
      endif
      go to 100

 10   n=ng-NGBASE-1
      if(n.ge.1 .and.n.le.30) then
         write(grid,1012) -n
 1012    format(i3.2)
      else if(n.ge.31 .and.n.le.60) then
         n=n-30
         write(grid,1022) -n
 1022    format('R',i3.2)
      else if(n.eq.61) then
         grid='RO'
      else if(n.eq.62) then
         grid='RRR'
      else if(n.eq.63) then
         grid='73'
      endif

 100  return
      end

