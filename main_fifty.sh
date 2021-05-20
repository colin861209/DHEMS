#!/bin/bash
truncate -s 0 ~/how/DHEMS/log/fifty_GHEMS.log
truncate -s 0 ~/how/DHEMS/log/fifty_LHEMS.log
echo "#### RUN no dr LHEMS: grid, GHEMS: grid, sell, ess, fc ####"
for i in {1..96}
do
   for j in {1..5}
   do
      echo "-------- RUN LEHMS at $j times --------"
      /home/hems/how/DHEMS/build/fifty_LHEMS >> /home/hems/how/DHEMS/log/fifty_LHEMS.log
   done
   wait
   echo "~~~~~~~~~~ RUN GEHMS at $i times ~~~~~~~~~~"
   /home/hems/how/DHEMS/build/fifty_GHEMS $Hydro_Price $weather >> /home/hems/how/DHEMS/log/fifty_GHEMS.log
done