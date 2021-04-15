#!/bin/bash
truncate -s 0 ~/how/DHEMS/log/*.log
echo "#### RUN dr2 & LHEMS grid + ess & GHEMS no fc ####"
for i in {1..96}
do
   for j in {1..5}
   do
      echo "-------- RUN LEHMS at $j times --------"
      /home/hems/how/DHEMS/build/LHEMS >> /home/hems/how/DHEMS/log/LHEMS.log
   done
   wait
   echo "~~~~~~~~~~ RUN GEHMS at $i times ~~~~~~~~~~"
   /home/hems/how/DHEMS/build/GHEMS $Hydro_Price $weather >> /home/hems/how/DHEMS/log/GHEMS.log
done