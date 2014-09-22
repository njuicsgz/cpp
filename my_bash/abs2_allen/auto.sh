#!  /bin/bash


export PATH=/sbin:/usr/sbin:/usr/local/sbin:/usr/local/bin:/usr/bin:/bin


workdir=$( cd `dirname $0` && pwd ) 
cd $workdir;

log=$workdir/log.d/log.$( date '+%F_%H%M' )
logd=$( dirname $log )
[[ -d $logd ]] || mkdir -p $logd

{ echo y; echo y; } | bash ./abs.sh -t 10 -T 10 -L 999999 | tee -a $log

echo "LOG: $log"


## . "$workdir/func-common.sh" 

##logmsg "now check log ..."
#echo "----------------------- NOT OK IP -----------------------------"
#grep -w "OPOK" "$log" | awk '{ print $1 }' | grep -wvf - abs-iplist | tee notok
#echo "-----------------------    DONE   -----------------------------"
