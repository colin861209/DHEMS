#!/bin/bash
truncate -s 0 ~/how/DHEMS/log_fifty/*.log

start_time=`date "+%Y-%m-%d %H:%M:%S"`
for i in {1..96}
do
   for j in {1..5}
   do
      echo "-------- RUN LEHMS at $j times --------"
      echo "----- LHEMS1 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS.log &
      echo "----- LHEMS2 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS2 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS2.log &
      echo "----- LHEMS3 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS3 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS3.log &
      echo "----- LHEMS4 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS4 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS4.log &
      echo "----- LHEMS5 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS5 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS5.log &
      echo "----- LHEMS6 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS6 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS6.log &
      echo "----- LHEMS7 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS7 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS7.log &
      echo "----- LHEMS8 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS8 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS8.log &
      echo "----- LHEMS9 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS9 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS9.log &
      echo "----- LHEMS10 -----"
      /home/hems/how/DHEMS/build/fifty_LHEMS10 >> /home/hems/how/DHEMS/log_fifty/fifty_LHEMS10.log 
      wait
   done
   wait
   echo "~~~~~~~~~~ RUN GEHMS at $i times ~~~~~~~~~~"
   /home/hems/how/DHEMS/build/fifty_GHEMS $Hydro_Price $weather >> /home/hems/how/DHEMS/log_fifty/fifty_GHEMS.log
done
end_time=`date "+%Y-%m-%d %H:%M:%S"`
duration=`echo $(($(date +%s -d "${end_time}") - $(date +%s -d "${start_time}"))) | awk '{t=split("60 s 60 m 24 h 999 d",a);for(n=1;n<t;n+=2){if($1==0)break;s=$1%a[n]a[n+1]s;$1=int($1/a[n])}print s}'`

echo -e "\nTime cost: $duration"