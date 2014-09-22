#! /bin/bash

## ----------------------------------------------------------

. "$( dirname $0 )/func-common.sh" || exit 1
. "$( dirname $0 )/bash_mt.sh"     || exit 1

cd "$WORKDIR" || exit 1

logd="$WORKDIR/log.d"
[[ -d $logd ]] || mkdir -p $logd

abs_iplist="$WORKDIR/abs-iplist"
abs_config="$WORKDIR/abs-config"
notok_iplist="$WORKDIR/notok"
logfile_all="$logd/logall.$(  date +%Y%m%d_%H%M%S )"

abs_max_job=30

run_abs()
{
    local line=$@
    local iplist ip log

    case $line in 
    [1-9]*)

        iplist=$( mktemp -f "$WORKDIR/tmp_iplist.d" )
        ip=$( echo $line | awk '{ print $1 }' )
        log=$logd/log.$ip

        echo "running [$ip]"
        echo "$line" > "$iplist"

        { echo y; echo y; } | bash ./abs.sh -t 60 -T 60 -L 999999 -l "$iplist" -c "$abs_config" > $log 2>&1

        dos2unix "$log" &> /dev/null
        
        if grep -wq "OPOK" "$log"; then
            :
        else
            cat "$iplist" >> "$notok_iplist"
        fi

        /bin/rm "$iplist"

        echo "finished [$ip]"
        ;;
    *)
        return 
        ;;
    esac        
} 


> "$notok_iplist"

mt_init "$abs_max_job"

while read line; do
    mt_run run_abs "$line" 
done < "$abs_iplist"

wait

msg "----------------------------------------------------------------"
msg "LOGFILE: $logfile_all"
msg "NOTOK:   $notok_iplist"
msg "----------------------------------------------------------------"

