#!/bin/bash
weather="cloudy"  
weatherString="Weather ${weather}" 
Hydro_Price="0.08"
Hydro_PriceString="FC Price ${Hydro_Price}" 
truncate -s 0 ~/how/DHEMS/log/*.log
echo "=-=-=-=-=-=-=-=-=- ${weatherString} -=-=-=-=-=-=-=-=-="
echo "=-=-=-=-=-=-=-=-=- ${Hydro_PriceString} -=-=-=-=-=-=-=-=-="
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