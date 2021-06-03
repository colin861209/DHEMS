#!/bin/bash
truncate -s 0 ~/how/DHEMS/log/LHEMS_dr2.log
truncate -s 0 ~/how/DHEMS/log/GHEMS_dr2.log
for i in {1..96}
do
   for j in {1..5}
   do
      echo "-------- RUN LEHMS at $j times --------"
      /home/hems/how/DHEMS/build/LHEMS_dr2 >> /home/hems/how/DHEMS/log/LHEMS_dr2.log
   done
   wait
   echo "~~~~~~~~~~ RUN GEHMS at $i times ~~~~~~~~~~"
   /home/hems/how/DHEMS/build/GHEMS_dr2 $Hydro_Price $weather >> /home/hems/how/DHEMS/log/GHEMS_dr2.log
done