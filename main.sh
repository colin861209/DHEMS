#!/bin/bash
weather="cloudy"  
weatherString="Weather ${weather}" 
Hydro_Price="0.1"
truncate -s 0 LHEMS.log
truncate -s 0 GHEMS.log
echo "=-=-=-=-=-=-=-=-=- ${weatherString} -=-=-=-=-=-=-=-=-="
for i in {1..96}
do
   for j in {1..5}
   do
      echo "=-=-=-=-=-=-=-=-=- RUN LEHMS at $j times -=-=-=-=-=-=-=-=-="
      /home/hems/how/DHEMS/build/LHEMS >> /home/hems/how/DHEMS/LHEMS.log
   done
   wait
   echo "=-=-=-=-=-=-=-=-=- RUN GEHMS at $i times -=-=-=-=-=-=-=-=-="
   /home/hems/how/DHEMS/build/GHEMS $Hydro_Price $weather >> /home/hems/how/DHEMS/GHEMS.log
done